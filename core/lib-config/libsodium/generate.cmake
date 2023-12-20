# Generate version.h file
set(SODIUM_LIBRARY_MINIMAL_DEF "")

# Prevent parent scope variable pollution
set(VERSION ${SODIUM_LIBRARY_VERSION_STRING})

configure_file(${libsodium_SOURCE_DIR}/src/libsodium/include/sodium/version.h.in ${libsodium_SOURCE_DIR}/src/libsodium/include/sodium/version.h @ONLY)