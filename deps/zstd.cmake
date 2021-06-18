set(D ${CMAKE_CURRENT_LIST_DIR})

set(ZSTD_PATH ${D}/zstd/lib)
include_directories(${ZSTD_PATH})
include_directories(${ZSTD_PATH}/common)
include_directories(${ZSTD_PATH}/compress)

add_library(zstd STATIC
${ZSTD_PATH}/common/entropy_common.c
${ZSTD_PATH}/common/error_private.c
${ZSTD_PATH}/common/fse_decompress.c
${ZSTD_PATH}/common/pool.c
${ZSTD_PATH}/common/threading.c
${ZSTD_PATH}/common/xxhash.c
${ZSTD_PATH}/common/zstd_common.c
${ZSTD_PATH}/compress/fse_compress.c
${ZSTD_PATH}/compress/huf_compress.c
${ZSTD_PATH}/compress/zstd_compress.c
${ZSTD_PATH}/compress/zstd_double_fast.c
${ZSTD_PATH}/compress/zstd_fast.c
${ZSTD_PATH}/compress/zstd_lazy.c
${ZSTD_PATH}/compress/zstd_ldm.c
${ZSTD_PATH}/compress/zstdmt_compress.c
${ZSTD_PATH}/compress/zstd_opt.c
${ZSTD_PATH}/decompress/huf_decompress.c
${ZSTD_PATH}/decompress/zstd_decompress.c)
