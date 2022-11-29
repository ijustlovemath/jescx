//
// Created by jcdej on 2022-11-20.
//
// From here: http://gareus.org/wiki/embedding_resources_in_executables (also available on wayback machine)

/**
 * Usage:
 * 1. Add the custom commands to CMakeLists.txt to create the object files from source text
 * 2. Add the object files to the list of source files in add_executable or add_library
 * 3. Replacing all non-alphanumeric characters from the source file name with underscores,
 *      find the embedded variable name. For example, "determine-basal.mjs" -> "determine_basal_mjs"
 *
 *    In your source file where you want to use the data, declare it using EXTLD, get u8 data pointer with LDVAR, length with LDLEN
 *
 *    EXTLD(determine_basal_mjs)
 *
 *    int main(void) {
 *      const unsigned char *data = LDVAR(determine_basal_mjs);
 *      const unsigned long = LDLEN(determine_basal_mjs); // this should be size_t
 */

#ifndef QUICKJS_LIBC_C_LD_MAGIC_H
#define QUICKJS_LIBC_C_LD_MAGIC_H

#ifdef __APPLE__
#include <mach-o/getsect.h>

#define EXTLD(NAME) \
  extern const unsigned char _section$__DATA__ ## NAME [];
#define LDVAR(NAME) _section$__DATA__ ## NAME
#define LDLEN(NAME) (getsectbyname("__DATA", "__" #NAME)->size)

#elif (defined __WIN32__)  /* mingw */

#define EXTLD(NAME) \
  extern const unsigned char binary_ ## NAME ## _start[]; \
  extern const unsigned char binary_ ## NAME ## _end[];
#define LDVAR(NAME) \
  binary_ ## NAME ## _start
#define LDLEN(NAME) \
  ((binary_ ## NAME ## _end) - (binary_ ## NAME ## _start))

#else /* gnu/linux ld */

#define EXTLD(NAME) \
  extern const unsigned char _binary_ ## NAME ## _start[]; \
  extern const unsigned char _binary_ ## NAME ## _end[];
#define LDVAR(NAME) \
  _binary_ ## NAME ## _start
#define LDLEN(NAME) \
  ((_binary_ ## NAME ## _end) - (_binary_ ## NAME ## _start))
#endif

#endif //QUICKJS_LIBC_C_LD_MAGIC_H
