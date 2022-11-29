## Automatically donwload and use module CPM.cmake
file(DOWNLOAD https://raw.githubusercontent.com/TheLartians/CPM.cmake/v0.26.2/cmake/CPM.cmake
        "${CMAKE_BINARY_DIR}/CPM.cmake")
include("${CMAKE_BINARY_DIR}/CPM.cmake")

#----------- Add dependencies --------------------------#

CPMAddPackage(
        NAME               quickjs
        GITHUB_REPOSITORY  bellard/quickjs
        GIT_TAG            2788d71e823b522b178db3b3660ce93689534e6d
        # DOWNLOAD_ONLY YES
)


# Add this directory where is this file (CMakeLists.txt) to include path.
include_directories( ${CMAKE_CURRENT_LIST_DIR} )

# =============== QuickJS settings ====================================#

include_directories( ${quickjs_SOURCE_DIR}/ )
message([TRACE] " quickjs source = ${quickjs_SOURCE_DIR} ")

file(GLOB quickjs_hpp ${quickjs_SOURCE_DIR}/*.h )

file(GLOB quickjs_src ${quickjs_SOURCE_DIR}/quickjs.c
        ${quickjs_SOURCE_DIR}/libregexp.c
        ${quickjs_SOURCE_DIR}/libunicode.c
        ${quickjs_SOURCE_DIR}/cutils.c
        ${quickjs_SOURCE_DIR}/quickjs-libc.c
        ${quickjs_SOURCE_DIR}/libbf.c
        )


add_library( quickjs ${quickjs_src} ${quickjs_hpp} )
target_compile_options( quickjs PRIVATE
        -MMD -MF
        -Wno-sign-compare
        -Wno-missing-field-initializers
        -Wundef -Wuninitialized
        -Wundef -Wuninitialized -Wwrite-strings -Wchar-subscripts
        -fPIC
        )
target_compile_definitions( quickjs PUBLIC
        CONFIG_BIGNUM=y
        CONFIG_VERSION="2021-03-27"
        DUMP_MODULE_RESOLVE=1
        _GNU_SOURCE
        )

if(UNIX)
    target_link_libraries( quickjs PRIVATE ${COMMON_LIBS})
endif()

add_custom_command(
    OUTPUT my-foo-lib.mjs
    COMMAND ./scripts/build-mjs.sh
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/foo.js
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_command(
    OUTPUT foo_lib_js.o
    COMMAND ld -r -b binary -o ${CMAKE_BINARY_DIR}/foo_lib_js.o my-foo-lib.mjs
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/my-foo-lib.mjs
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

#set_property(SOURCE foo_lib_js.o PROPERTY GENERATED 1)
set_property(SOURCE foo_lib.c APPEND PROPERTY OBJECT_DEPENDS foo_lib_js.o)

add_library(foo_lib SHARED foo_lib.c ${CMAKE_BINARY_DIR}/foo_lib_js.o)

target_link_libraries(foo_lib quickjs ${COMMON_LIBS})

add_executable(main main.c)
target_link_libraries(main foo_lib)

