#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(libhttp, 1.0.0)
AC_CONFIG_SRCDIR([src/http.c])
AM_INIT_AUTOMAKE([foreign no-dist no-installinfo no-installman -Wall])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h stddef.h stdint.h string.h errno.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([ \
    malloc \
    free \
    memcpy \
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

AC_CONFIG_FILES([Makefile \
                 libhttp.pc \
                 src/Makefile \
                 test/Makefile])
AC_OUTPUT
