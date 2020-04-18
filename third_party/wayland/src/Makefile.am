if BUILD_DOCS
SUBDIRS = doc
endif

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

AM_CPPFLAGS =					\
	-I$(top_builddir)/src			\
	-I$(top_srcdir)/src			\
	-I$(top_builddir)/protocol

AM_CFLAGS = $(GCC_CFLAGS)

aclocaldir = $(datadir)/aclocal
dist_aclocal_DATA = wayland-scanner.m4

dist_pkgdata_DATA =				\
	wayland-scanner.mk			\
	protocol/wayland.xml			\
	protocol/wayland.dtd

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA =

bin_PROGRAMS = wayland-scanner
wayland_scanner_SOURCES = src/scanner.c
wayland_scanner_CFLAGS = $(EXPAT_CFLAGS) $(LIBXML_CFLAGS) $(AM_CFLAGS)
wayland_scanner_LDADD = $(EXPAT_LIBS) $(LIBXML_LIBS) libwayland-util.la
pkgconfig_DATA += src/wayland-scanner.pc

if DTD_VALIDATION
wayland_scanner_SOURCES += src/dtddata.S
endif
src/dtddata.o: protocol/wayland.dtd

if USE_HOST_SCANNER
wayland_scanner = wayland-scanner
else
$(BUILT_SOURCES) : wayland-scanner
wayland_scanner = $(top_builddir)/wayland-scanner
endif

libwayland_util_la_CFLAGS = $(AM_CFLAGS)
libwayland_util_la_SOURCES =			\
	src/wayland-util.c			\
	src/wayland-util.h

noinst_LTLIBRARIES = libwayland-util.la

if ENABLE_LIBRARIES
noinst_LTLIBRARIES += libwayland-private.la
lib_LTLIBRARIES = libwayland-server.la libwayland-client.la

libwayland_private_la_CFLAGS = $(FFI_CFLAGS) $(AM_CFLAGS)
libwayland_private_la_SOURCES =			\
	src/connection.c			\
	src/wayland-os.c			\
	src/wayland-os.h			\
	src/wayland-private.h

include_HEADERS =				\
	src/wayland-util.h			\
	src/wayland-server.h			\
	src/wayland-server-core.h		\
	src/wayland-client.h			\
	src/wayland-client-core.h		\
	src/wayland-egl.h			\
	src/wayland-egl-core.h			\
	src/wayland-version.h

nodist_include_HEADERS =			\
	protocol/wayland-server-protocol.h	\
	protocol/wayland-client-protocol.h

libwayland_server_la_CFLAGS = $(FFI_CFLAGS) $(AM_CFLAGS) -pthread
libwayland_server_la_LIBADD = $(FFI_LIBS) libwayland-private.la libwayland-util.la -lrt -lm
libwayland_server_la_LDFLAGS = -version-info 1:0:1
libwayland_server_la_SOURCES =			\
	src/wayland-server.c			\
	src/wayland-shm.c			\
	src/event-loop.c

nodist_libwayland_server_la_SOURCES =		\
	protocol/wayland-server-protocol.h	\
	protocol/wayland-protocol.c

libwayland_client_la_CFLAGS = $(FFI_CFLAGS) $(AM_CFLAGS) -pthread
libwayland_client_la_LIBADD = $(FFI_LIBS) libwayland-private.la libwayland-util.la -lrt -lm
libwayland_client_la_LDFLAGS = -version-info 3:0:3
libwayland_client_la_SOURCES =			\
	src/wayland-client.c

nodist_libwayland_client_la_SOURCES =		\
	protocol/wayland-client-protocol.h	\
	protocol/wayland-protocol.c

pkgconfig_DATA += src/wayland-client.pc src/wayland-server.pc

protocol/%-protocol.c : $(top_srcdir)/protocol/%.xml
	$(AM_V_GEN)$(MKDIR_P) $(dir $@) && $(wayland_scanner) code < $< > $@

protocol/%-server-protocol.h : $(top_srcdir)/protocol/%.xml
	$(AM_V_GEN)$(MKDIR_P) $(dir $@) && $(wayland_scanner) server-header < $< > $@

protocol/%-client-protocol.h : $(top_srcdir)/protocol/%.xml
	$(AM_V_GEN)$(MKDIR_P) $(dir $@) && $(wayland_scanner) client-header < $< > $@

protocol/%-server-protocol-core.h : $(top_srcdir)/protocol/%.xml
	$(AM_V_GEN)$(MKDIR_P) $(dir $@) && $(wayland_scanner) server-header -c < $< > $@

protocol/%-client-protocol-core.h : $(top_srcdir)/protocol/%.xml
	$(AM_V_GEN)$(MKDIR_P) $(dir $@) && $(wayland_scanner) client-header -c < $< > $@

BUILT_SOURCES =					\
	$(nodist_libwayland_server_la_SOURCES)	\
	$(nodist_libwayland_client_la_SOURCES)	\
	$(nodist_headers_test_SOURCES)

CLEANFILES = $(BUILT_SOURCES) doc/doxygen/doxygen_sqlite3.db
DISTCLEANFILES = src/wayland-version.h
EXTRA_DIST = src/wayland-version.h.in



lib_LTLIBRARIES += libwayland-cursor.la

include_HEADERS += cursor/wayland-cursor.h

libwayland_cursor_la_SOURCES =			\
	cursor/wayland-cursor.c			\
	cursor/os-compatibility.c		\
	cursor/os-compatibility.h		\
	cursor/cursor-data.h			\
	cursor/xcursor.c			\
	cursor/xcursor.h
libwayland_cursor_la_LIBADD = libwayland-client.la

pkgconfig_DATA += cursor/wayland-cursor.pc

libwayland_cursor_la_CFLAGS =			\
	$(AM_CFLAGS)				\
	-I$(top_builddir)/src			\
	-I$(top_srcdir)/src			\
	-DICONDIR=\"$(ICONDIR)\"


built_test_programs =				\
	array-test				\
	client-test				\
	display-test				\
	connection-test				\
	event-loop-test				\
	fixed-test				\
	interface-test				\
	list-test				\
	map-test				\
	os-wrappers-test			\
	sanity-test				\
	socket-test				\
	queue-test				\
	signal-test				\
	newsignal-test				\
	resources-test				\
	message-test				\
	headers-test				\
	compositor-introspection-test		\
	protocol-logger-test

if ENABLE_CPP_TEST
built_test_programs += cpp-compile-test
endif

AM_TESTS_ENVIRONMENT =							\
	export WAYLAND_SCANNER='$(top_builddir)/wayland-scanner'	\
	TEST_DATA_DIR='$(top_srcdir)/tests/data'			\
	TEST_OUTPUT_DIR='$(top_builddir)/tests/output'			\
	SED=$(SED)							\
	;

TESTS = $(built_test_programs)			\
	tests/scanner-test.sh

noinst_PROGRAMS =				\
	$(built_test_programs)			\
	exec-fd-leak-checker			\
	fixed-benchmark

noinst_LTLIBRARIES += libtest-runner.la

libtest_runner_la_SOURCES =			\
	tests/test-runner.c			\
	tests/test-runner.h			\
	tests/test-helpers.c			\
	tests/test-compositor.h			\
	tests/test-compositor.c
libtest_runner_la_LIBADD =			\
	libwayland-private.la			\
	libwayland-util.la			\
	libwayland-client.la			\
	libwayland-server.la			\
	-lrt -ldl $(FFI_LIBS)


array_test_SOURCES = tests/array-test.c
array_test_LDADD = libtest-runner.la
client_test_SOURCES = tests/client-test.c
client_test_LDADD = libtest-runner.la
display_test_SOURCES = tests/display-test.c
display_test_LDADD = libtest-runner.la
connection_test_SOURCES = tests/connection-test.c
connection_test_LDADD = libtest-runner.la
event_loop_test_SOURCES = tests/event-loop-test.c
event_loop_test_LDADD = libtest-runner.la
fixed_test_SOURCES = tests/fixed-test.c
fixed_test_LDADD = libtest-runner.la
interface_test_SOURCES = tests/interface-test.c
interface_test_LDADD = libtest-runner.la
list_test_SOURCES = tests/list-test.c
list_test_LDADD = libtest-runner.la
map_test_SOURCES = tests/map-test.c
map_test_LDADD = libtest-runner.la
sanity_test_SOURCES = tests/sanity-test.c
sanity_test_LDADD = libtest-runner.la
socket_test_SOURCES = tests/socket-test.c
socket_test_LDADD = libtest-runner.la
queue_test_SOURCES = tests/queue-test.c
queue_test_LDADD = libtest-runner.la
signal_test_SOURCES = tests/signal-test.c
signal_test_LDADD = libtest-runner.la
# wayland-server.c is needed here to access wl_priv_* functions
newsignal_test_SOURCES = tests/newsignal-test.c src/wayland-server.c
newsignal_test_LDADD = libtest-runner.la
resources_test_SOURCES = tests/resources-test.c
resources_test_LDADD = libtest-runner.la
message_test_SOURCES = tests/message-test.c
message_test_LDADD = libtest-runner.la
compositor_introspection_test_SOURCES = tests/compositor-introspection-test.c
compositor_introspection_test_LDADD = libtest-runner.la
protocol_logger_test_SOURCES = tests/protocol-logger-test.c
protocol_logger_test_LDADD = libtest-runner.la
headers_test_SOURCES = tests/headers-test.c \
		       tests/headers-protocol-test.c \
		       tests/headers-protocol-core-test.c
headers_test_LDADD = libtest-runner.la
nodist_headers_test_SOURCES =			\
	protocol/wayland-server-protocol-core.h	\
	protocol/wayland-client-protocol-core.h

if ENABLE_CPP_TEST
cpp_compile_test_SOURCES = tests/cpp-compile-test.cpp
endif

fixed_benchmark_SOURCES = tests/fixed-benchmark.c
fixed_benchmark_LDADD = libtest-runner.la

os_wrappers_test_SOURCES = tests/os-wrappers-test.c
os_wrappers_test_LDADD = libtest-runner.la

exec_fd_leak_checker_SOURCES = tests/exec-fd-leak-checker.c
exec_fd_leak_checker_LDADD = libtest-runner.la

EXTRA_DIST += tests/scanner-test.sh			\
	tests/data/example.xml				\
	tests/data/example-client.h			\
	tests/data/example-server.h			\
	tests/data/example-code.c			\
	tests/data/small.xml				\
	tests/data/small-code.c				\
	tests/data/small-client.h			\
	tests/data/small-server.h			\
	tests/data/small-code-core.c			\
	tests/data/small-client-core.h			\
	tests/data/small-server-core.h

tests/scanner-test.sh: $(top_builddir)/wayland-scanner

clean-local:
	-rm -rf tests/output

endif #ENABLE_LIBRARIES
