set(D ${CMAKE_CURRENT_LIST_DIR})

set(SQLITE3_PATH ${D}/sqlite3)
include_directories(${SQLITE3_PATH})

add_library(sqlite3 STATIC
  ${SQLITE3_PATH}/sqlite3.c)
target_compile_options(sqlite3 PRIVATE -DSQLITE_OMIT_LOAD_EXTENSION=1)
