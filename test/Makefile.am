AM_CPPFLAGS = -I../src
AM_CFLAGS = @WARNINGS@ -D TESTS

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

check_PROGRAMS += test_status
test_status_LDFLAGS = -L../src -lhttp
test_status_SOURCES = test_status.c

check_PROGRAMS += test_reason
test_reason_LDFLAGS = -L../src -lhttp
test_reason_SOURCES = test_reason.c

TESTS = $(check_PROGRAMS)
