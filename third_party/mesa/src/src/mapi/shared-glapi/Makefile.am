# Used by OpenGL ES or when --enable-shared-glapi is specified

TOP = $(top_srcdir)
GLAPI = $(top_srcdir)/src/mapi/glapi
include $(top_srcdir)/src/mapi/mapi/sources.mak

lib_LTLIBRARIES = libglapi.la
libglapi_la_SOURCES = $(MAPI_GLAPI_FILES)
libglapi_la_LIBADD = $(PTHREAD_LIBS) $(SELINUX_LIBS)
libglapi_la_LDFLAGS = -no-undefined

include $(GLAPI)/gen/glapi_gen.mk
glapi_mapi_tmp.h : $(GLAPI)/gen/gl_and_es_API.xml $(glapi_gen_mapi_deps)
	$(call glapi_gen_mapi,$<,shared-glapi)

BUILT_SOURCES = glapi_mapi_tmp.h
CLEANFILES = $(BUILT_SOURCES)

AM_CFLAGS = $(PTHREAD_CFLAGS)
AM_CPPFLAGS =							\
	$(DEFINES)						\
	-I$(top_srcdir)/include					\
	-I$(top_srcdir)/src/mapi				\
	-I$(top_builddir)/src/mapi                              \
	-DMAPI_MODE_GLAPI					\
	-DMAPI_ABI_HEADER=\"shared-glapi/glapi_mapi_tmp.h\"

all-local: libglapi.la
	$(MKDIR_P) $(top_builddir)/$(LIB_DIR)
	ln -f .libs/libglapi.so.0.0.0 $(top_builddir)/$(LIB_DIR)/libglapi.so.0.0.0
	ln -sf libglapi.so.0.0.0 $(top_builddir)/$(LIB_DIR)/libglapi.so.0
	ln -sf libglapi.so.0 $(top_builddir)/$(LIB_DIR)/libglapi.so
