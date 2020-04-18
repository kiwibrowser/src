AUTOMAKE_OPTIONS = subdir-objects

lib_LTLIBRARIES = libOpenCL.la

libOpenCL_la_LDFLAGS = \
	$(LLVM_LDFLAGS) \
	-version-number 1:0

libOpenCL_la_LIBADD = \
	$(top_builddir)/src/gallium/state_trackers/clover/libclover.la \
	$(top_builddir)/src/gallium/auxiliary/libgallium.a \
	$(GALLIUM_PIPE_LOADER_LIBS) $(LIBUDEV_LIBS) \
	-ldl \
	-lclangCodeGen \
	-lclangFrontendTool \
	-lclangFrontend \
	-lclangDriver \
	-lclangSerialization \
	-lclangCodeGen \
	-lclangParse \
	-lclangSema \
	-lclangAnalysis \
	-lclangRewrite \
	-lclangAST \
	-lclangLex \
	-lclangBasic \
	-lclangEdit \
	$(LLVM_LIBS)


libOpenCL_la_SOURCES =

# Force usage of a C++ linker
nodist_EXTRA_libOpenCL_la_SOURCES = dummy.cpp

PIPE_SRC_DIR = $(top_srcdir)/src/gallium/targets/pipe-loader

# Provide compatibility with scripts for the old Mesa build system for
# a while by putting a link to the driver into /lib of the build tree.
all-local: libOpenCL.la
	@$(MAKE) -C $(PIPE_SRC_DIR)
	$(MKDIR_P) $(top_builddir)/$(LIB_DIR)
	ln -f .libs/libOpenCL.so* $(top_builddir)/$(LIB_DIR)/

install-exec-local:
	@$(MAKE) -C $(PIPE_SRC_DIR) PIPE_INSTALL_DIR=$(OPENCL_LIB_INSTALL_DIR) install

clean-local:
	@$(MAKE) -C $(PIPE_SRC_DIR) clean

# FIXME: Remove when the rest of Gallium is converted to automake.
TOP=$(top_builddir)
default: all
