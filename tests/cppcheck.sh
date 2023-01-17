#!/bin/bash
source $(dirname "$0")/../util/bash-base.sh

echo "[HUSARNET BS] Running cppcheck"

cppcheck --error-exitcode=1 --enable=all --inline-suppr --force \
    -itodo \
    -iesp32 \
    ${base_dir}/daemon \
    ${base_dir}/core \
    ${tests_base}/unit

    # I've tried enabling those and they seem to be generating so much nose that's not worth it
    # -I ${build_base}/unix/amd64/tempIncludes/ \
    # -I /app/build/unix/arm64/_deps/better_enums-src \

    # -I /app/build/unix/arm64/_deps/nlohmann_json-src/include \
    # -I /zig/zig-linux-x86_64-0.9.1/lib/libcxx/include/ \
    # -I /zig/zig-linux-x86_64-0.9.1/lib/libcxx/include/__support/musl/ \
    # -I /zig/zig-linux-x86_64-0.9.1/lib/libc/include/any-linux-any/ \
    # -I /zig/zig-linux-x86_64-0.9.1/lib/libc/include/any-windows-any \
    # -I /zig/zig-linux-x86_64-0.9.1/lib/libc/include/generic-musl/ \
    # -I /zig/zig-linux-x86_64-0.9.1/lib/libc/include/x86_64-linux-musl \
    # -I /zig/zig-linux-x86_64-0.9.1/lib/libc/musl/arch/x86_64/ \
    # -I /zig/zig-linux-x86_64-0.9.1/lib/libc/musl/include/ \
