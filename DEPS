# This file manages the dependencies used by Skia developers for local, stand-alone Skia builds and
# Skia testing infrastructure. The versions specified by the commit hashes represent the revisions
# we happen to be currently testing. Skia provides no endorsement or recommendation on the revision
# to use for these libraries.

use_relative_paths = True

vars = {
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling different
  # dependencies without interference from each other.
  'infra_revision': '84b209ba05f1d48d38eae9ebeb2ff0ebf05956c2',

  # ninja CIPD package version.
  # https://chrome-infra-packages.appspot.com/p/infra/3pp/tools/ninja
  'ninja_version': 'version:2@1.12.1.chromium.4',

  # googlefonts_testdata CIPD package version
  # https://chrome-infra-packages.appspot.com/p/chromium/third_party/googlefonts_testdata/
  'googlefonts_testdata_version': 'version:20230913',

  # Pre-built task drivers from this repo, used for CI.
  'task_drivers_revision': 'git_revision:b5d31abb7bc772a69f800de45783768768437675',
}

# If you modify this file, you will need to regenerate the Bazel version of this file (bazel/deps.bzl).
# To do so, run:
#     bazelisk run //bazel/deps_parser
#
# To apply the changes for the GN build, you will need to resync the git repositories using:
#     ./tools/git-sync-deps
deps = {
  "third_party/externals/brotli"                 : "https://skia.googlesource.com/external/github.com/google/brotli.git@6d03dfbedda1615c4cba1211f8d81735575209c8",
  "third_party/externals/d3d12allocator"         : "https://skia.googlesource.com/external/github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator.git@169895d529dfce00390a20e69c2f516066fe7a3b",
  "third_party/externals/expat"                  : "https://chromium.googlesource.com/external/github.com/libexpat/libexpat.git@6154446fccefbf3ca644894f598969113b0c7bcd",
  "third_party/externals/freetype"               : "https://chromium.googlesource.com/chromium/src/third_party/freetype2.git@264b5fbf5b912b39f98d038bf75d39be0a73f21b",
  "third_party/externals/harfbuzz"               : "https://chromium.googlesource.com/external/github.com/harfbuzz/harfbuzz.git@9cb1fee51069b206effb4736e443b038d230789d",
  "third_party/externals/icu"                    : "https://chromium.googlesource.com/chromium/deps/icu.git@364118a1d9da24bb5b770ac3d762ac144d6da5a4",
  "third_party/externals/libjpeg-turbo"          : "https://chromium.googlesource.com/chromium/deps/libjpeg_turbo.git@e14cbfaa85529d47f9f55b0f104a579c1061f9ad",
  "third_party/externals/libpng"                 : "https://skia.googlesource.com/third_party/libpng.git@d5515b5b8be3901aac04e5bd8bd5c89f287bcd33",
  "third_party/externals/libwebp"                : "https://chromium.googlesource.com/webm/libwebp.git@845d5476a866141ba35ac133f856fa62f0b7445f",
  "third_party/externals/vulkanmemoryallocator"  : "https://chromium.googlesource.com/external/github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator@a6bfc237255a6bac1513f7c1ebde6d8aed6b5191",
  "third_party/externals/spirv-cross"            : "https://chromium.googlesource.com/external/github.com/KhronosGroup/SPIRV-Cross@b8fcf307f1f347089e3c46eb4451d27f32ebc8d3",
  #"third_party/externals/v8"                     : "https://chromium.googlesource.com/v8/v8.git@5f1ae66d5634e43563b2d25ea652dfb94c31a3b4",
  "third_party/externals/wuffs"                  : "https://skia.googlesource.com/external/github.com/google/wuffs-mirror-release-c.git@e3f919ccfe3ef542cfc983a82146070258fb57f8",
  "third_party/externals/zlib"                   : "https://chromium.googlesource.com/chromium/src/third_party/zlib@646b7f569718921d7d4b5b8e22572ff6c76f2596",

  # Dawn + transitive deps required by third_party/dawn/build_dawn.py when
  # skia_use_dawn=true. Versions track infra/bots/deps/deps_gen.go for m148.
  # Skia's git-sync-deps doesn't support gclient-style 'condition' gating, so
  # non-graphite users also pay the (shallow) clone cost for these repos.
  "third_party/externals/dawn"                    : "https://dawn.googlesource.com/dawn.git@d641a1d08b2048314c6e245b66614f6deb4aacc7",
  "third_party/externals/abseil-cpp"              : "https://chromium.googlesource.com/chromium/src/third_party/abseil-cpp.git@2a7d49fc392cad55159d68d98aa3648bc89795d3",
  "third_party/externals/jinja2"                  : "https://chromium.googlesource.com/chromium/src/third_party/jinja2.git@c3027d884967773057bf74b957e3fea87e5df4d7",
  "third_party/externals/markupsafe"              : "https://chromium.googlesource.com/chromium/src/third_party/markupsafe.git@4256084ae14175d38a3ff7d739dca83ae49ccec6",
  "third_party/externals/glslang"                 : "https://chromium.googlesource.com/external/github.com/KhronosGroup/glslang.git@1d47ffa8ac4374a19b302021e216a20f22a3de92",
  "third_party/externals/vulkan-headers"          : "https://chromium.googlesource.com/external/github.com/KhronosGroup/Vulkan-Headers.git@afe9eb980aa928a66d1c9c06f38c55dd59868720",
  "third_party/externals/vulkan-utility-libraries": "https://chromium.googlesource.com/external/github.com/KhronosGroup/Vulkan-Utility-Libraries.git@48b1fd1a65e436bae806cb6180c9338846b9de97",
  "third_party/externals/webgpu-headers"          : "https://chromium.googlesource.com/external/github.com/webgpu-native/webgpu-headers.git@706853a9da45b8e89b7ea005aa267294d115f8ce",
  "third_party/externals/egl-registry"            : "https://skia.googlesource.com/external/github.com/KhronosGroup/EGL-Registry.git@b055c9b483e70ecd57b3cf7204db21f5a06f9ffe",
  "third_party/externals/opengl-registry"         : "https://skia.googlesource.com/external/github.com/KhronosGroup/OpenGL-Registry.git@14b80ebeab022b2c78f84a573f01028c96075553",
  "third_party/externals/spirv-headers"           : "https://skia.googlesource.com/external/github.com/KhronosGroup/SPIRV-Headers.git@6dd7ba990830f7c15ac1345ff3b43ef6ffdad216",
  "third_party/externals/spirv-tools"             : "https://skia.googlesource.com/external/github.com/KhronosGroup/SPIRV-Tools.git@2d14d2e76aa7de72404b17078eda15c20a6a0389",
  "third_party/externals/swiftshader"             : "https://swiftshader.googlesource.com/SwiftShader.git@89556131bf9d48af3c5c9fbb9a3322e706da89a3",

  'bin': {
    'packages': [
      {
        'package': 'skia/tools/sk/${{platform}}',
        'version': 'git_revision:' + Var('infra_revision'),
      },
      {
        'package': 'infra/3pp/tools/ninja/${{platform}}',
        'version': Var('ninja_version'),
      }
    ],
    'dep_type': 'cipd',
  },

  'task_drivers': {
    'packages': [
      {
        'package': 'skia/tools/bazel_build/${{platform}}',
        'version': Var('task_drivers_revision'),
      },
    ],
    'dep_type': 'cipd',
    'condition': 'False',
  },

  'infra/skia-infra': {
    'url': 'https://skia.googlesource.com/buildbot.git@' + Var('infra_revision'),
    'condition': 'False',
  },
}
