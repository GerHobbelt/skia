/*
* Copyright 2023 Google LLC
*
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE file.
*/

#include "src/gpu/graphite/vk/VulkanRenderPass.h"

#include "include/gpu/graphite/vk/VulkanGraphiteTypes.h"
#include "src/gpu/graphite/RenderPassDesc.h"
#include "src/gpu/graphite/Texture.h"
#include "src/gpu/graphite/vk/VulkanCommandBuffer.h"
#include "src/gpu/graphite/vk/VulkanGraphiteUtils.h"
#include "src/gpu/graphite/vk/VulkanSharedContext.h"
#include "src/gpu/graphite/vk/VulkanTexture.h"

#include <limits>

namespace skgpu::graphite {

namespace {

void add_subpass_info_to_key(ResourceKey::Builder& builder,
                             int& builderIdx,
                             const VulkanRenderPass::Metadata& rpData) {
    SkASSERT(rpData.fColorAttachIndex >= 0); // We expect to always have a valid color attachment

    // TODO: Fetch actual attachment reference and index information for each
    // subpass from RenderPassDesc. For now, determine subpass data based upon whether we are
    // loading from MSAA or not.
    const int mainSubpassIdx = rpData.fLoadMSAAFromResolve ? 1 : 0;
    // Assign a smaller value to represent VK_ATTACHMENT_UNUSED.
    static constexpr int kAttachmentUnused = std::numeric_limits<uint8_t>::max();

    // The following key structure assumes that we only have up to one reference of each type per
    // subpass and that attachments are indexed in order of color, resolve, depth/stencil, then
    // input attachments. These indices are statically defined in the VulkanRenderPass header file.
    for (int j = 0; j < rpData.subpassCount(); j++) {
        if (j == mainSubpassIdx) {
            // For keying purposes, if the renderpass will not use the input attachment, we mark its
            // input attachment as unused. This differentiates it from compatible-only renderpasses
            // and those that require dst reads so that it can use
            // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL for the color attachment. However, the
            // actual subpass will still have an input attachment ref to preserve renderpass
            // compatibility to minimize pipeline compiles.
            builder[builderIdx++] =
                    (rpData.fColorAttachIndex   >= 0 ? SkTo<uint8_t>(rpData.fColorAttachIndex)
                                                     : kAttachmentUnused)        |
                    ((rpData.fColorResolveIndex >= 0 ? SkTo<uint8_t>(rpData.fColorResolveIndex)
                                                     : kAttachmentUnused) << 8)  |
                    ((rpData.fDepthStencilIndex >= 0 ? SkTo<uint8_t>(rpData.fDepthStencilIndex)
                                                     : kAttachmentUnused) << 16) |
                    ((rpData.fUsesInputAttachment    ? SkTo<uint8_t>(rpData.fColorAttachIndex)
                                                     : kAttachmentUnused) << 24);
        } else { // Loading MSAA from resolve subpass
            builder[builderIdx++] =
                    static_cast<uint8_t>(rpData.fColorAttachIndex) | // color attachment
                    (kAttachmentUnused << 8)                       | // No color resolve attachment
                    (kAttachmentUnused << 16)                      | // No depth/stencil attachment
                    // The input attachment for the load subpass is the color resolve texture.
                    (static_cast<uint8_t>(rpData.fColorResolveIndex) << 24);
        }
    }

    // TODO: Query RenderPassDesc for subpass dependency information & populate the key accordingly.
    // For now, we know that the only subpass dependency will be that expected for loading MSAA from
    // resolve.
    for (int i = 0; i < rpData.subpassDependencyCount(); i++) {
        builder[builderIdx++] = 0 | (mainSubpassIdx << 8); // srcSubpass, dstSubpass
        builder[builderIdx++] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // srcStageMask
        builder[builderIdx++] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // dstStageMask
        builder[builderIdx++] = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;          // srcAccessMask
        builder[builderIdx++] = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |          // dstAccessMask
                                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        builder[builderIdx++] = VK_DEPENDENCY_BY_REGION_BIT;                   // dependencyFlags
    }
}

} // anonymous namespace

VulkanRenderPass::Metadata::Metadata(const RenderPassDesc& rpDesc, bool compatibleOnly) {
    fLoadMSAAFromResolve = RenderPassDescWillLoadMSAAFromResolve(rpDesc);

    // TODO: These represent the attachment refs of the main subpass, RenderPassDesc will allow for
    // more attachments and subpasses in the future. Subpasses will not have more than 1 color,
    // resolve, or depth+stencil attachment but could have more input attachments.

    // When creating a compatible-only renderpass, we assume that it might need to use the
    // self-referencing input attachment for dst reads (final determination is made per DrawPass
    // based on what was recorded into it). For non-compatible-only renderpasses, mark the input
    // attachment as unused. This will allow the VkRenderPass to use a more optimal layout, although
    // it will still have an input attachment to remain compatible.
    fUsesInputAttachment = compatibleOnly ||
                           rpDesc.fDstReadStrategy == DstReadStrategy::kReadFromInput;

    // Accumulate attachments into a container to mimic future structure in RenderPassDesc
    if (rpDesc.fColorAttachment.fFormat != TextureFormat::kUnsupported) {
        fColorAttachIndex = fAttachments.size();
        fAttachments.push_back(rpDesc.fColorAttachment);
    } else {
        fColorAttachIndex = -1;
    }
    if (rpDesc.fColorResolveAttachment.fFormat != TextureFormat::kUnsupported) {
        fColorResolveIndex = fAttachments.size();
        fAttachments.push_back(rpDesc.fColorResolveAttachment);
    } else {
        fColorResolveIndex = -1;
    }
    if (rpDesc.fDepthStencilAttachment.fFormat != TextureFormat::kUnsupported) {
        fDepthStencilIndex = fAttachments.size();
        fAttachments.push_back(rpDesc.fDepthStencilAttachment);
    } else {
        fDepthStencilIndex = -1;
    }

    if (compatibleOnly) {
        // Reset all load/store ops on the attachments since those do not affect compatibility.
        for (AttachmentDesc& d : fAttachments) {
            d.fLoadOp = LoadOp::kDiscard;
            d.fStoreOp = StoreOp::kDiscard;
        }
    }
}

bool VulkanRenderPass::Metadata::operator==(const Metadata& other) const {
    return fLoadMSAAFromResolve == other.fLoadMSAAFromResolve &&
           fColorAttachIndex == other.fColorAttachIndex &&
           fColorResolveIndex == other.fColorResolveIndex &&
           fDepthStencilIndex == other.fDepthStencilIndex &&
           fUsesInputAttachment == other.fUsesInputAttachment &&
           fAttachments == other.fAttachments;
}

int VulkanRenderPass::Metadata::keySize() const {
    // The key will be formed such that bigger-picture items (such as the total attachment count)
    // will be near the front of the key to more quickly eliminate incompatible keys. Each
    // renderpass key will start with the total number of attachments associated with it
    // followed by how many subpasses and subpass dependencies the renderpass has. Packed together,
    // these will use one uint32.
    int num32DataCnt = 1;
    SkASSERT(static_cast<uint32_t>(fAttachments.size()) <= (1u << 8));
    SkASSERT(static_cast<uint32_t>(this->subpassCount()) <= (1u << 8));
    SkASSERT(static_cast<uint32_t>(this->subpassDependencyCount()) <= (1u << 8));

    // The key will then contain format, sample count, and load/store ops for each attachment.
    // It packs up to 2 attachments per uint32_t
    num32DataCnt += (fAttachments.size() + 1) / 2;

    // Then, subpass information will be added in the form of attachment reference indices. Reserve
    // one int32 for each possible attachment reference type, of which there are 4.
    // There are 4 possible attachment reference types. Pack all 4 attachment reference indices into
    // one uint32.
    num32DataCnt += this->subpassCount();
    // Each subpass dependency will be allotted 6 int32s to store all its pertinent information.
    num32DataCnt += 6 * this->subpassDependencyCount();
    return num32DataCnt;
}

void VulkanRenderPass::Metadata::addToKey(ResourceKey::Builder& builder, int& builderIdx) {
    builder[builderIdx++] = fAttachments.size()  |
                            (this->subpassCount() << 8) |
                            (this->subpassDependencyCount() << 16);

    // Iterate through each renderpass attachment to add its information. Each attachment is packed
    // into 16 bits, so two attachments per key field
    for (int i = 0; i < fAttachments.size(); i++) {
        AttachmentDesc desc = fAttachments[i];
        uint16_t descKey = static_cast<uint8_t>(desc.fFormat) << 8 |
                           static_cast<uint8_t>(desc.fLoadOp) << 6 |
                           static_cast<uint8_t>(desc.fStoreOp) << 4 |
                           desc.fSampleCount;
        uint32_t& keySlot = builder[builderIdx + i/2];
        if (i % 2 == 0) {
            keySlot = descKey;
        } else {
            keySlot = (descKey << 16) | keySlot;
        }
    }
    builderIdx += (fAttachments.size() + 1) / 2;

    // TODO: Currently subpasses and their dependencies are keyed for maximum simplicity and ability
    // to represent all Vulkan features. This makes for a large key size. It is unlikely that the
    // full flexibility of Vulkan subpass dependencies will be exposed in the generalized subpasses
    // of RenderPassDesc, so if those are sufficient for the Vulkan backend as well then we may be
    // able to reduce the key size here.
    add_subpass_info_to_key(builder, builderIdx, *this);
}

namespace {

void setup_vk_attachment_description(VkAttachmentDescription* outAttachment,
                                     const AttachmentDesc& desc,
                                     const VkImageLayout initialLayout,
                                     const VkImageLayout finalLayout) {
    static_assert((int) LoadOp::kLoad     == (int) VK_ATTACHMENT_LOAD_OP_LOAD);
    static_assert((int) LoadOp::kClear    == (int) VK_ATTACHMENT_LOAD_OP_CLEAR);
    static_assert((int) LoadOp::kDiscard  == (int) VK_ATTACHMENT_LOAD_OP_DONT_CARE);
    static_assert((int) StoreOp::kStore   == (int) VK_ATTACHMENT_STORE_OP_STORE);
    static_assert((int) StoreOp::kDiscard == (int) VK_ATTACHMENT_STORE_OP_DONT_CARE);

    memset(outAttachment, 0, sizeof(VkAttachmentDescription));

    VkAttachmentLoadOp vkLoadOp = static_cast<VkAttachmentLoadOp>(desc.fLoadOp);
    VkAttachmentStoreOp vkStoreOp = static_cast<VkAttachmentStoreOp>(desc.fStoreOp);

    outAttachment->flags = 0;
    outAttachment->format = TextureFormatToVkFormat(desc.fFormat);
    VkSampleCountFlagBits sampleCount;
    SkAssertResult(SampleCountToVkSampleCount(desc.fSampleCount, &sampleCount));
    outAttachment->samples = sampleCount;
    switch (initialLayout) {
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        case VK_IMAGE_LAYOUT_GENERAL:
            outAttachment->loadOp = vkLoadOp;
            outAttachment->storeOp = vkStoreOp;
            outAttachment->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            outAttachment->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            // The loadOp and storeOp refer to the depth part of the attachment and the stencil*Ops
            // refer to the stencil bits in the attachment.
            outAttachment->loadOp = vkLoadOp;
            outAttachment->storeOp = vkStoreOp;
            outAttachment->stencilLoadOp = vkLoadOp;
            outAttachment->stencilStoreOp = vkStoreOp;
            break;
        default:
            SK_ABORT("Unexpected attachment layout");
    }
    outAttachment->initialLayout = initialLayout;
    outAttachment->finalLayout = finalLayout == VK_IMAGE_LAYOUT_UNDEFINED ? initialLayout
                                                                          : finalLayout;
}

void populate_attachment_refs(const VulkanRenderPass::Metadata& rpMetadata,
                              skia_private::TArray<VkAttachmentDescription>& descs,
                              VkAttachmentReference& colorRef,
                              VkAttachmentReference& resolveRef,
                              VkAttachmentReference& resolveLoadInputRef,
                              VkAttachmentReference& depthStencilRef,
                              VkAttachmentReference& inputAttachRef) {
    static constexpr VkAttachmentReference kUnused{VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_UNDEFINED};

    if (rpMetadata.fColorAttachIndex >= 0) {
        // If reading from the dst as an input attachment, we must use VK_IMAGE_LAYOUT_GENERAL
        // for the color attachment description. Use a general image layout for all compatible
        // renderpasses as well.
        //
        // This is necessary in order to avoid validation layer errors. Despite attachment layouts
        // not being an aspect of RP compatibility, subpass description attachment
        // indices are. So to maintain compatibility between RPs that do and do not read
        // from the dst, we always assign the main subpass's input attachment to be the color
        // attachment. However, using VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL for an input
        // attachment reference triggers validation layer errors (even if the the RP is
        // compatible-only or, in the case where no dst read is needed, never end up actually
        // reading from the input attachment).
        //
        // Therefore, only use VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL for the color attachment
        // description for full RPs that do not read from the dst as an input attachment. The
        // layout in the subpass remains VK_IMAGE_LAYOUT_GENERAL, but it is expected that the
        // drivers can optimize the transition from COLOR_ATTACHMENT_OPTIMAL -> GENERAL -> and back
        // to be a no-op so renderpasses that do not actually read from the dst should have
        // performance similar to if there was no input attachment self-dependency at all.
        colorRef.layout = VK_IMAGE_LAYOUT_GENERAL;
        VkImageLayout layout =
                rpMetadata.fUsesInputAttachment ? VK_IMAGE_LAYOUT_GENERAL
                                                : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        colorRef.attachment = descs.size();
        VkAttachmentDescription& vkColorAttachDesc = descs.push_back();
        setup_vk_attachment_description(
                &vkColorAttachDesc,
                rpMetadata.fAttachments[rpMetadata.fColorAttachIndex],
                layout,
                layout);

        if (rpMetadata.fColorResolveIndex >= 0) {
            VkImageLayout initialResolveLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            VkImageLayout finalResolveLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            resolveRef.attachment = descs.size();
            resolveRef.layout = finalResolveLayout; // Attachment ref expects final layout
            if (rpMetadata.fLoadMSAAFromResolve) {
                // If we are loading MSAA from resolve, we do not expect to later treat the resolve
                // texture as a dst that we can read as an input attachment. Therefore, we do not
                // have to worry about using a general layout for dst reads.
                initialResolveLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                resolveLoadInputRef.attachment = resolveRef.attachment;
                resolveLoadInputRef.layout = initialResolveLayout;
            } else {
                resolveLoadInputRef = kUnused;
            }

            VkAttachmentDescription& vkResolveAttachDesc = descs.push_back();
            setup_vk_attachment_description(
                    &vkResolveAttachDesc,
                    rpMetadata.fAttachments[rpMetadata.fColorResolveIndex],
                    initialResolveLayout,
                    finalResolveLayout);
        } else {
            resolveRef = resolveLoadInputRef = kUnused;
        }
    } else {
        SkASSERT(false);
        colorRef = resolveRef = resolveLoadInputRef = kUnused;
    }

    if (rpMetadata.fDepthStencilIndex >= 0) {
        depthStencilRef.attachment = descs.size();
        depthStencilRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription& vkDepthStencilAttachDesc = descs.push_back();
        setup_vk_attachment_description(
                &vkDepthStencilAttachDesc,
                rpMetadata.fAttachments[rpMetadata.fDepthStencilIndex],
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    } else {
        depthStencilRef = kUnused;
    }

    // No VkAttachmentDescription is needed for the inputAttachRef because it merely points to one
    // of the existing attachments. Assign the input attachment ref's attachment to be the color
    // attachment even for renderpasses that do not use an input attachment on the main subpass.
    // Normally, we would want to use VK_ATTACHMENT_UNUSED in such cases, but always assigning it to
    // be the color attachment even when unused allows for compatible-only renderpasses to be shared
    // for pipelines that do read from the dst and those that do not.
    inputAttachRef.attachment = colorRef.attachment;

    // Even though attachment layouts are not an aspect of renderpass compatibility, indicating the
    // usage of a layout incompatible with input attachment usage preemptively triggers validation
    // layer errors since we do not use VK_ATTACHMENT_UNUSED, so use VK_IMAGE_LAYOUT_GENERAL here.
    // A full, non-compatible only renderpass that performs no dst reads can still use
    // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL for the color attachment without validation layer
    // complaints so long as the input attachment is not actually used by the command buffer.
    // On non-tiler GPUs, most drivers have optimizations surrounding this case (an attachment
    // reference that is not actually read from using a layout that differs from the
    // VkAttachmentDescription's initial and final layout), meaning performance should not be
    // notably impacted.
    inputAttachRef.layout = VK_IMAGE_LAYOUT_GENERAL;
}

void populate_subpass_dependencies(skia_private::STArray<2, VkSubpassDependency>& deps,
                                   bool loadMSAAFromResolve) {
    const int mainSubpassIdx = loadMSAAFromResolve ? 1 : 0;

    // Adding a single subpass self-dependency for color attachments is basically free, so apply
    // one to every RenderPass which has an input attachment on the main subpass. This is useful
    // because it means that as we perform draw calls, if we encounter a draw that uses a blend
    // operation requiring a dst read, we can avoid having to switch RenderPasses.
    VkSubpassDependency& selfDependency = deps.push_back();
    selfDependency.srcSubpass = mainSubpassIdx;
    selfDependency.dstSubpass = mainSubpassIdx;
    selfDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    selfDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    selfDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    selfDependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    selfDependency.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;

    // If loading MSAA from resolve, enforce that subpass goes first with a subpass dependency.
    if (loadMSAAFromResolve) {
        VkSubpassDependency& dependency = deps.push_back();
        dependency.srcSubpass = 0;
        dependency.dstSubpass = mainSubpassIdx;
        dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask =
                VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
}

void populate_subpass_descs(skia_private::TArray<VkSubpassDescription>& descs,
                            const VkAttachmentReference& colorRef,
                            const VkAttachmentReference& resolveRef,
                            const VkAttachmentReference& resolveLoadInputRef,
                            const VkAttachmentReference& depthStencilRef,
                            const VkAttachmentReference& inputAttachRef) {
    // If loading MSAA from resolve, add the additional subpass to do so.
    if (resolveLoadInputRef.attachment != VK_ATTACHMENT_UNUSED) {
        VkSubpassDescription& loadSubpassDesc = descs.push_back();
        memset(&loadSubpassDesc, 0, sizeof(VkSubpassDescription));
        loadSubpassDesc.flags = 0;
        loadSubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        loadSubpassDesc.inputAttachmentCount = 1;
        loadSubpassDesc.pInputAttachments = &resolveLoadInputRef;
        loadSubpassDesc.colorAttachmentCount = 1;
        loadSubpassDesc.pColorAttachments = &colorRef;
        loadSubpassDesc.pResolveAttachments = nullptr;
        loadSubpassDesc.pDepthStencilAttachment = nullptr;
        loadSubpassDesc.preserveAttachmentCount = 0;
        loadSubpassDesc.pPreserveAttachments = nullptr;
    }

    VkSubpassDescription& mainSubpassDesc = descs.push_back();
    memset(&mainSubpassDesc, 0, sizeof(VkSubpassDescription));
    mainSubpassDesc.flags = 0;
    mainSubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    // Always include one input attachment on the main subpass which can optionally be used or not
    mainSubpassDesc.inputAttachmentCount = 1;
    mainSubpassDesc.pInputAttachments = &inputAttachRef;
    mainSubpassDesc.colorAttachmentCount = 1;
    mainSubpassDesc.pColorAttachments = &colorRef;
    mainSubpassDesc.pResolveAttachments = &resolveRef;
    mainSubpassDesc.pDepthStencilAttachment = &depthStencilRef;
    mainSubpassDesc.preserveAttachmentCount = 0;
    mainSubpassDesc.pPreserveAttachments = nullptr;
}

} // anonymous namespace

sk_sp<VulkanRenderPass> VulkanRenderPass::Make(const VulkanSharedContext* context,
                                               const Metadata& rpMetadata) {
    // Set up attachment descriptions + references. Declare them before having a helper populate
    // their values so we can reference them later during RP creation.
    skia_private::TArray<VkAttachmentDescription> attachmentDescs;
    VkAttachmentReference colorRef;
    VkAttachmentReference resolveRef;
    VkAttachmentReference resolveLoadInputRef;
    VkAttachmentReference depthStencilRef;
    VkAttachmentReference inputAttachRef;
    populate_attachment_refs(rpMetadata,
                             attachmentDescs,
                             colorRef,
                             resolveRef,
                             resolveLoadInputRef,
                             depthStencilRef,
                             inputAttachRef);

    // Assemble subpass information before creating the renderpass. Each renderpass has at least one
    // subpass dependency (self-dependency for reading the dst texture). If loading MSAA from
    // resolve, that adds another subpass and an additional dependency.
    skia_private::STArray<2, VkSubpassDescription> subpassDescs;
    populate_subpass_descs(subpassDescs,
                           colorRef,
                           resolveRef,
                           resolveLoadInputRef,
                           depthStencilRef,
                           inputAttachRef);
    skia_private::STArray<2, VkSubpassDependency> dependencies;
    populate_subpass_dependencies(dependencies, rpMetadata.fLoadMSAAFromResolve);

    // Create VkRenderPass
    VkRenderPassCreateInfo renderPassInfo;
    memset(&renderPassInfo, 0, sizeof(VkRenderPassCreateInfo));
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.pNext = nullptr;
    renderPassInfo.flags = 0;
    renderPassInfo.subpassCount = subpassDescs.size();
    renderPassInfo.pSubpasses = subpassDescs.begin();
    renderPassInfo.dependencyCount = dependencies.size();
    renderPassInfo.pDependencies = dependencies.data();
    renderPassInfo.attachmentCount = attachmentDescs.size();
    renderPassInfo.pAttachments = attachmentDescs.data();

    VkResult result;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VULKAN_CALL_RESULT(context,
                       result,
                       CreateRenderPass(context->device(), &renderPassInfo, nullptr, &renderPass));
    if (result != VK_SUCCESS) {
        return nullptr;
    }
    VkExtent2D granularity;
    VULKAN_CALL(context->interface(), GetRenderAreaGranularity(context->device(),
                                                               renderPass,
                                                               &granularity));
    return sk_sp<VulkanRenderPass>(new VulkanRenderPass(context, renderPass, granularity));
}

VulkanRenderPass::VulkanRenderPass(const VulkanSharedContext* context,
                                   VkRenderPass renderPass,
                                   VkExtent2D granularity)
        : Resource(context,
                   Ownership::kOwned,
                   /*gpuMemorySize=*/0)
        , fSharedContext(context)
        , fRenderPass(renderPass)
        , fGranularity(granularity) {}

void VulkanRenderPass::freeGpuData() {
    VULKAN_CALL(fSharedContext->interface(),
                DestroyRenderPass(fSharedContext->device(), fRenderPass, nullptr));
}

} // namespace skgpu::graphite
