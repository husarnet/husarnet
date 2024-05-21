# Generate version.h file
set(LIBNL_VERSION "${LIBNL_VER_MAJ}.${LIBNL_VER_MIN}.${LIBNL_VER_MIC}")
set(LIBNL_STRING "libnl ${LIBNL_VERSION}")

# Prevent parent scope variable pollution
set(PACKAGE_STRING ${LIBNL_STRING})
set(PACKAGE_VERSION ${LIBNL_VERSION})

set(MAJ_VERSION ${LIBNL_VER_MAJ})
set(MIN_VERSION ${LIBNL_VER_MIN})
set(MIC_VERSION ${LIBNL_VER_MIC})

set(LT_CURRENT ${LIBNL_CURRENT})
set(LT_REVISION ${LIBNL_REVISION})
set(LT_AGE ${LIBNL_AGE})

configure_file(${libnl_SOURCE_DIR}/include/netlink/version.h.in ${libnl_SOURCE_DIR}/include/netlink/version.h @ONLY)

# Generate config.h file
# This file originally used autoconf to generate define feature flags, but cmake automatically populates them for us
# It is left for compatibility with existing code and time.h include workaround (see config.h.in file)

configure_file(${CMAKE_CURRENT_LIST_DIR}/config.h.in ${libnl_SOURCE_DIR}/include/config.h @ONLY)
