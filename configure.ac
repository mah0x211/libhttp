#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(libhttp, 1.0.1)
AC_CONFIG_SRCDIR([src/http.c])
AM_INIT_AUTOMAKE([foreign no-dist no-installinfo no-installman -Wall])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
AM_PROG_AR
AC_PROG_LIBTOOL

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h stddef.h stdint.h string.h errno.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_CHECK_TYPES([uintptr_t, uint64_t, uint32_t, uint16_t, uint8_t, size_t])

# Checks for library functions.
AC_CHECK_FUNCS([ \
    malloc \
    calloc \
    free \
])

#
# debug option
#
AC_ARG_ENABLE(
    [debug],
    AS_HELP_STRING([--enable-debug], [enable debug build.]),
    [ENABLE_DEBUG=$enableval], [ENABLE_DEBUG=no]
)
AS_IF([test "$ENABLE_DEBUG" != "no"],
    [ AC_DEFINE([NDEBUG], [1], [Define if you enabled debug]),
      AC_SUBST([CFLAGS], ["-g -O0"]) ],
    [ AC_SUBST([CFLAGS], ["-O3"]) ]
)

#
# warnings
#
AC_SUBST([WARNINGS], ["-Wall -Wno-trigraphs -Wmissing-field-initializers -Wmissing-prototypes -Wreturn-type -Wmissing-braces -Wparentheses -Wswitch -Wunused-function -Wunused-label -Wunused-parameter -Wunused-variable -Wunused-value -Wuninitialized -Wunknown-pragmas -Wshadow -Wsign-compare -Wextra -Wwrite-strings -Wcast-qual -Wredundant-decls -Wmissing-declarations -Wpointer-arith -Wstrict-prototypes -Wnested-externs -Wformat=2"])


AC_CONFIG_FILES([Makefile \
                 libhttp.pc \
                 src/Makefile \
                 test/Makefile \
                 bench/Makefile \
                 example/Makefile])
AC_OUTPUT
