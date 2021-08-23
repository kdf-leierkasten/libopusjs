#!/bin/bash

set -e

# Create build dir
mkdir -p build
cd build

# If opus does not exist, build it
if [ ! -d opus ]; then
    # Get source
    git clone https://github.com/xiph/opus.git opus
    cd opus
    git checkout tags/v1.2.1
    rm -rf .git

    # Build
    ./autogen.sh
    emconfigure ./configure --disable-rtcd --disable-intrinsics --disable-shared --enable-static
    emmake make -j"$(nproc)"

    cd ..
fi

# Build libopusjs
em++ ../api.cpp -Iopus/include -Lopus/.libs -lopus --pre-js ../preapi.js --post-js ../api.js -O3 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 --memory-init-file 0 -s EXPORTED_FUNCTIONS="['_free', '_malloc']" -o libopus.js
