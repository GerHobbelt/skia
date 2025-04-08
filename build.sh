#!/bin/sh
set -e -x

APT=${APT:-0}
if [ "${APT}" = 1 ]; then
    sudo apt update -y
    sudo apt install -y libgl1-mesa-dev libglx-dev libglvnd-core-dev curl git clang build-essential python3 ninja-build libfontconfig1-dev libfreetype-dev libexpat1-dev libjpeg-turbo8-dev libpng-dev libwebp-dev zlib1g-dev libicu-dev libharfbuzz-dev
fi

rm -rf out/build-bundle || true
rm -rf out/build-system || true

git submodule update --init
python3 tools/git-sync-deps

bin/gn gen out/build-bundle --args='is_official_build=true is_debug=false cc="clang" cxx="clang++" extra_cflags=["-Wno-error"] target_os="linux" target_cpu="x64" skia_use_system_expat=false skia_use_system_freetype2=false skia_use_system_libjpeg_turbo=false skia_use_system_libpng=false skia_use_system_libwebp=false skia_use_system_zlib=false skia_use_system_icu=false skia_use_system_harfbuzz=false skia_use_dng_sdk=false'
bin/gn gen out/build-system --args='is_official_build=true is_debug=false cc="clang" cxx="clang++" extra_cflags=["-Wno-error"] target_os="linux" target_cpu="x64" skia_use_system_expat=true skia_use_system_freetype2=true skia_use_system_libjpeg_turbo=true skia_use_system_libpng=true skia_use_system_libwebp=true skia_use_system_zlib=true skia_use_system_icu=true skia_use_system_harfbuzz=true skia_use_dng_sdk=false'

ninja -C out/build-bundle skia
ninja -C out/build-system skia
