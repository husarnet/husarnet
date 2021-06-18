#!/bin/bash
cd "$(dirname "$0")"/..

flags="-DWITH_ZSTD -DSODIUM_STATIC -DSODIUM_DLL_EXPORT -USODIUM_DISABLE_WEAK_FUNCTIONS -UNDEBUG"
flags="$flags -UESP_PLATFORM -D__linux__ -U_MSC_VER -U_WIN32 -U__ANDROID__"
includes="-Icore -Icommon"
# --enable=performance --enable=style
checks="--enable=portability,warning,style --suppress=useInitializationList --suppress=passedByValue --suppress=stlcstrReturn --suppress=useStlAlgorithm stlcstrParam --inline-suppr"

core_src=$(echo common/*.cpp core/*.cpp)
cppcheck --error-exitcode=1 --quiet $checks $includes $flags $core_src
