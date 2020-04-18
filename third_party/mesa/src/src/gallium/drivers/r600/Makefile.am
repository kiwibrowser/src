include Makefile.sources

noinst_LIBRARIES = libr600.a

AM_CFLAGS = \
	-I$(top_srcdir)/src/gallium/include \
	-I$(top_srcdir)/src/gallium/auxiliary \
	-I$(top_srcdir)/src/gallium/drivers \
	-I$(top_srcdir)/include \
	$(RADEON_CFLAGS) \
	$(DEFINES) \
	$(PIC_FLAGS) \
	$(VISIBILITY_CFLAGS)

libr600_a_SOURCES = \
	$(C_SOURCES)

if NEED_RADEON_GALLIUM

# This is a hack until we can move the backend into the LLVM project.
# We need to use mklib, because it splits up libradeon.a into object files
# so that we can link it with the r600 objects.
libr600_a_AR = $(top_srcdir)/bin/mklib -o r600 -static

libr600_a_SOURCES += \
	$(LLVM_C_SOURCES) \
	$(LLVM_CXX_SOURCES)

libr600_a_LIBADD = \
	$(top_builddir)/src/gallium/drivers/radeon/libradeon.a

AM_CFLAGS += \
	$(LLVM_CFLAGS) \
	-I$(top_srcdir)/src/gallium/drivers/radeon/

AM_CXXFLAGS= \
	$(LLVM_CXXFLAGS)
else
libr600_a_AR = $(AR) $(ARFLAGS)
endif

if USE_R600_LLVM_COMPILER
AM_CFLAGS += \
	-DR600_USE_LLVM
endif

if HAVE_GALLIUM_COMPUTE
AM_CFLAGS += \
	-DHAVE_OPENCL
endif
