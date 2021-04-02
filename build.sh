#!/bin/bash
# usage: ./build.sh [flags]
# asm.js release flags: -O2 --memory-init-file 0
# wasm release flags: -O2 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1
em++ api.cpp -Iopus/include -Lopus/.libs -lopus --pre-js preapi.js --post-js api.js -O3 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 --memory-init-file 0 -s EXPORTED_FUNCTIONS="['_free', '_malloc']" -o libopus.js
