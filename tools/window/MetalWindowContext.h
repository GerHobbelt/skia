/*
 * Copyright 2019 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef MetalWindowContext_DEFINED
#define MetalWindowContext_DEFINED

#include "include/core/SkRefCnt.h"
#include "include/core/SkSurface.h"
#include "include/ports/SkCFObject.h"
#include "src/gpu/ganesh/mtl/GrMtlTypesPriv.h"

#include "tools/window/WindowContext.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

namespace skwindow::internal {

class MetalWindowContext : public WindowContext {
public:
    sk_sp<SkSurface> getBackbufferSurface() override;

    bool isValid() override { return fValid; }

    void setDisplayParams(std::unique_ptr<const DisplayParams> params) override;

protected:
    MetalWindowContext(std::unique_ptr<const DisplayParams>);
    // This should be called by subclass constructor. It is also called when window/display
    // parameters change. This will in turn call onInitializeContext().
    void initializeContext();
    virtual bool onInitializeContext() = 0;

    // This should be called by subclass destructor. It is also called when window/display
    // parameters change prior to initializing a new Metal context. This will in turn call
    // onDestroyContext().
    void destroyContext();
    virtual void onDestroyContext() = 0;

    void onSwapBuffers() override;

    bool                        fValid;
    sk_cfp<id<MTLDevice>>       fDevice;
    sk_cfp<id<MTLCommandQueue>> fQueue;
    CAMetalLayer*               fMetalLayer;
    GrMTLHandle                 fDrawableHandle;
};

}   // namespace skwindow::internal

#endif
