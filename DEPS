use_relative_paths = True

vars = {
  "checkout_chromium": False,

  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling different
  # dependencies without interference from each other.
  'sk_tool_revision': 'git_revision:0056074ae4f8d12d03946b968daea6a73943017f',
}

deps = {
  "third_party/externals/d3d12allocator"  : "https://skia.googlesource.com/external/github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator.git@169895d529dfce00390a20e69c2f516066fe7a3b",
  "third_party/externals/expat"           : "https://chromium.googlesource.com/external/github.com/libexpat/libexpat.git@a28238bdeebc087071777001245df1876a11f5ee",
  "third_party/externals/freetype"        : "https://chromium.googlesource.com/chromium/src/third_party/freetype2.git@5e9caaee7885cbc82f9f821bbec7f6c86f25b33a",
  "third_party/externals/harfbuzz"        : "https://chromium.googlesource.com/external/github.com/harfbuzz/harfbuzz.git@a8b7f1880412c7f0c9ecdada0a4935011816c7dc",
  "third_party/externals/icu"             : "https://chromium.googlesource.com/chromium/deps/icu.git@a0718d4f121727e30b8d52c7a189ebf5ab52421f",
  "third_party/externals/libgifcodec"     : "https://skia.googlesource.com/libgifcodec@fd59fa92a0c86788dcdd84d091e1ce81eda06a77",
  "third_party/externals/libjpeg-turbo"   : "https://chromium.googlesource.com/chromium/deps/libjpeg_turbo.git@02959c3ee17abacfd1339ec22ea93301292ffd56",
  "third_party/externals/libpng"          : "https://skia.googlesource.com/third_party/libpng.git@386707c6d19b974ca2e3db7f5c61873813c6fe44",
  "third_party/externals/libwebp"         : "https://chromium.googlesource.com/webm/libwebp.git@69c7f16111ec582bf1e7cb4d0d4f8d127e28a715",
  "third_party/externals/piex"            : "https://android.googlesource.com/platform/external/piex.git@bb217acdca1cc0c16b704669dd6f91a1b509c406",
  "third_party/externals/spirv-cross"     : "https://chromium.googlesource.com/external/github.com/KhronosGroup/SPIRV-Cross@0e2880ab990e79ce6cc8c79c219feda42d98b1e8",
  #"third_party/externals/v8"              : "https://chromium.googlesource.com/v8/v8.git@5f1ae66d5634e43563b2d25ea652dfb94c31a3b4",
  "third_party/externals/zlib"            : "https://chromium.googlesource.com/chromium/src/third_party/zlib@c876c8f87101c5a75f6014b0f832499afeb65b73",

  "../src": {
    "url": "https://chromium.googlesource.com/chromium/src.git@8fa4c936281662f1ad1c97ffece6000052108b6a",
    "condition": "checkout_chromium",
  },

  'bin': {
    'packages': [
      {
        'package': 'skia/tools/sk/${{platform}}',
        'version': Var('sk_tool_revision'),
      }
    ],
    'dep_type': 'cipd',
  },
}

recursedeps = [
  "../src",
]

gclient_gn_args_from = 'src'
