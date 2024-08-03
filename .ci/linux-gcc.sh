#!/bin/bash -ex

if [ "$TARGET" = "appimage" ]; then
    # Bundle required QT wayland libraries
    export EXTRA_QT_PLUGINS="waylandcompositor"
    export EXTRA_PLATFORM_PLUGINS="libqwayland-egl.so;libqwayland-generic.so"
else
    # For the linux-fresh verification target, verify compilation without PCH as well.
    export EXTRA_CMAKE_FLAGS=(-DCITRA_USE_PRECOMPILED_HEADERS=OFF)
fi

mkdir build && cd build
cmake .. -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER_LAUNCHER=ccache \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DCMAKE_CXX_COMPILER=gcc-14 \
    -DCMAKE_C_COMPILER=gcc-14 \
    -DCMAKE_LINKER=/usr/bin/gold \
    -DCMAKE_CXX_FLAGS="-O2 -lstdc++ -fuse-ld=gold" \
    -DCMAKE_C_FLAGS="-O2 -lstdc++ -fuse-ld=gold" \
    "${EXTRA_CMAKE_FLAGS[@]}" \
    -DENABLE_LTO=OFF \
    -DENABLE_TESTS=OFF \
    -DENABLE_QT_TRANSLATION=ON \
    -DCITRA_ENABLE_COMPATIBILITY_REPORTING=ON \
    -DUSE_DISCORD_PRESENCE=ON
ninja
strip -s bin/Release/*

if [ "$TARGET" = "appimage" ]; then
    ninja bundle
    # TODO: Our AppImage environment currently uses an older ccache version without the verbose flag.
    ccache -s
else
    ccache -s -v
fi

ctest -VV -C Release
