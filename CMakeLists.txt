cmake_minimum_required(VERSION 3.14)
project(jescx)

set(COMMON_LIBS m pthread dl)

include_directories(quickjs)
include_directories(include)
add_subdirectory(src)
