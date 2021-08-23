#!/bin/bash

function on_exit_with_error {
    echo "Build script failed because one of the commands exited with an error."
#    rm libopus.js libopus.wasm
}

set -e
trap on_exit_with_error ERR

# Move into directory of the script first as it assumes cwd
cd "$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

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

# Insert fix to work with typescript.
# This is for documentation purposes, might not work forever.
# In case libopus.js changes significantly, the grep command will kill the script because of set -e
to_replace_regex='if\(Module\["ENVIRONMENT"]!=="NODE"\)libopus=Module;$'
grep -E "$to_replace_regex" libopus.js > /dev/null 2>&1
sed -i -E "s/$to_replace_regex/const libopus=Module;export{libopus}/" libopus.js
