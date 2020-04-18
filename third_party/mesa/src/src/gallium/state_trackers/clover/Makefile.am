AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = \
	$(GALLIUM_PIPE_LOADER_DEFINES) \
	-DMESA_VERSION=\"$(MESA_VERSION)\" \
	-DPIPE_SEARCH_DIR=\"$(OPENCL_LIB_INSTALL_DIR)\" \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/gallium/include \
	-I$(top_srcdir)/src/gallium/drivers \
	-I$(top_srcdir)/src/gallium/auxiliary \
	-I$(top_srcdir)/src/gallium/winsys \
	-I$(srcdir)

noinst_LTLIBRARIES = libclover.la libcltgsi.la libclllvm.la

libcltgsi_la_CXXFLAGS = \
	-std=c++0x

libcltgsi_la_SOURCES = \
	tgsi/compiler.cpp

libclllvm_la_CXXFLAGS = \
	-std=c++98 \
	$(LLVM_CPPFLAGS) \
	$(DEFINES) \
	-DLIBCLC_INCLUDEDIR=\"$(LIBCLC_INCLUDEDIR)/\" \
	-DLIBCLC_LIBEXECDIR=\"$(LIBCLC_LIBEXECDIR)/\" \
	-DCLANG_RESOURCE_DIR=\"$(CLANG_RESOURCE_DIR)\"

libclllvm_la_SOURCES = \
	llvm/invocation.cpp

libclover_la_CXXFLAGS = \
	-std=c++0x

libclover_la_LIBADD = \
	libcltgsi.la libclllvm.la

libclover_la_SOURCES = \
	core/base.hpp \
	core/compat.hpp \
	core/compiler.hpp \
	core/geometry.hpp \
	core/device.hpp \
	core/device.cpp \
	core/context.hpp \
	core/context.cpp \
	core/queue.hpp \
	core/queue.cpp \
	core/format.hpp \
	core/format.cpp \
	core/memory.hpp \
	core/memory.cpp \
	core/resource.hpp \
	core/resource.cpp \
	core/sampler.hpp \
	core/sampler.cpp \
	core/event.hpp \
	core/event.cpp \
	core/program.hpp \
	core/program.cpp \
	core/kernel.hpp \
	core/kernel.cpp \
	core/module.hpp \
	core/module.cpp \
	api/util.hpp \
	api/platform.cpp \
	api/device.cpp \
	api/context.cpp \
	api/queue.cpp \
	api/memory.cpp \
	api/transfer.cpp \
	api/sampler.cpp \
	api/event.cpp \
	api/program.cpp \
	api/kernel.cpp

cldir = $(includedir)/CL
cl_HEADERS = \
	$(top_srcdir)/include/CL/cl.h \
	$(top_srcdir)/include/CL/cl_ext.h \
	$(top_srcdir)/include/CL/cl_gl.h \
	$(top_srcdir)/include/CL/cl_gl_ext.h \
	$(top_srcdir)/include/CL/cl_platform.h \
	$(top_srcdir)/include/CL/opencl.h \
	$(top_srcdir)/include/CL/cl.hpp
