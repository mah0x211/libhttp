AM_CPPFLAGS = -I../src
AM_CFLAGS = -D TESTS

check_PROGRAMS = test_method
test_method_LDFLAGS = -L../src -lhttp
test_method_SOURCES = test_method.c

check_PROGRAMS += test_version
test_version_LDFLAGS = -L../src -lhttp
test_version_SOURCES = test_version.c

check_PROGRAMS += test_uri
test_uri_LDFLAGS = -L../src -lhttp
test_uri_SOURCES = test_uri.c

check_PROGRAMS += test_header
test_header_LDFLAGS = -L../src -lhttp
test_header_SOURCES = test_header.c


TESTS = $(check_PROGRAMS)
