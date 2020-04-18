# Makefile for building pipe driver shared libraries.
#
# Input variables: PIPE_INSTALL_DIR, PIPE_PREFIX (optional)
#
TOP = ../../../..
include $(TOP)/configs/current

PIPE_PREFIX ?= pipe_

PIPE_CPPFLAGS = \
	-DGALLIUM_RBUG \
	-DGALLIUM_TRACE \
	-DGALLIUM_GALAHAD \
	-I$(TOP)/include \
	-I$(TOP)/src/gallium/auxiliary \
	-I$(TOP)/src/gallium/drivers \
	-I$(TOP)/src/gallium/include \
	-I$(TOP)/src/gallium/winsys

PIPE_LIBS = \
	$(TOP)/src/gallium/drivers/identity/libidentity.a \
	$(TOP)/src/gallium/drivers/galahad/libgalahad.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/rbug/librbug.a \
	$(GALLIUM_AUXILIARIES)

PIPE_SYS = $(LIBDRM_LIBS) -lm -lpthread $(DLOPEN_LIBS)

PIPE_CFLAGS = $(LIBDRM_CFLAGS)

PIPE_LDFLAGS = -Wl,--no-undefined

# i915 pipe driver
i915_LIBS = \
	$(TOP)/src/gallium/winsys/i915/drm/libi915drm.a \
	$(TOP)/src/gallium/drivers/i915/libi915.a
i915_SYS = $(INTEL_LIBS)

# nouveau pipe driver
nouveau_LIBS = \
	$(TOP)/src/gallium/winsys/nouveau/drm/libnouveaudrm.a \
	$(TOP)/src/gallium/drivers/nv30/libnv30.a \
	$(TOP)/src/gallium/drivers/nv50/libnv50.a \
	$(TOP)/src/gallium/drivers/nvc0/libnvc0.a \
	$(TOP)/src/gallium/drivers/nouveau/libnouveau.a
nouveau_SYS = $(NOUVEAU_LIBS)

# r300 pipe driver
r300_LIBS = \
	$(TOP)/src/gallium/winsys/radeon/drm/libradeonwinsys.a \
	$(TOP)/src/gallium/drivers/r300/libr300.a
r300_SYS += $(RADEON_LIBS)

# r600 pipe driver
r600_LIBS = \
	$(TOP)/src/gallium/winsys/radeon/drm/libradeonwinsys.a \
	$(TOP)/src/gallium/drivers/r600/libr600.a
r600_SYS += $(RADEON_LIBS)

# radeonsi pipe driver
radeonsi_LIBS = \
	$(TOP)/src/gallium/winsys/radeon/drm/libradeonwinsys.a \
	$(TOP)/src/gallium/drivers/radeonsi/libradeonsi.a
radeonsi_SYS += $(RADEON_LIBS)

# vmwgfx pipe driver
vmwgfx_LIBS = \
	$(TOP)/src/gallium/winsys/svga/drm/libsvgadrm.a \
	$(TOP)/src/gallium/drivers/svga/libsvga.a

ifneq ($(findstring llvmpipe,$(GALLIUM_DRIVERS_DIRS)),)
   swrast_LIBS = $(TOP)/src/gallium/drivers/llvmpipe/libllvmpipe.a
   PIPE_CFLAGS += -DGALLIUM_LLVMPIPE
else ifneq ($(findstring softpipe,$(GALLIUM_DRIVERS_DIRS)),)
   swrast_LIBS = $(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a
   PIPE_CFLAGS += -DGALLIUM_SOFTPIPE
endif

# LLVM
ifeq ($(MESA_LLVM),1)
   PIPE_SYS += $(LLVM_LIBS)
   PIPE_LDFLAGS += $(LLVM_LDFLAGS)
endif

# determine the targets/sources
_PIPE_TARGETS_CC =
_PIPE_TARGETS_CXX =
PIPE_SOURCES =

ifneq ($(findstring i915/drm,$(GALLIUM_WINSYS_DIRS)),)
   _PIPE_TARGETS_CC += $(PIPE_PREFIX)i915.so
   PIPE_SOURCES += pipe_i915.c
endif

ifneq ($(findstring nouveau/drm,$(GALLIUM_WINSYS_DIRS)),)
   _PIPE_TARGETS_CXX += $(PIPE_PREFIX)nouveau.so
   PIPE_SOURCES += pipe_nouveau.c
endif

ifneq ($(findstring radeon/drm,$(GALLIUM_WINSYS_DIRS)),)
ifneq ($(findstring r300,$(GALLIUM_DRIVERS_DIRS)),)
   _PIPE_TARGETS_CC += $(PIPE_PREFIX)r300.so
   PIPE_SOURCES += pipe_r300.c
endif
endif

ifneq ($(findstring radeon/drm,$(GALLIUM_WINSYS_DIRS)),)
ifneq ($(findstring r600,$(GALLIUM_DRIVERS_DIRS)),)
   _PIPE_TARGETS_CC += $(PIPE_PREFIX)r600.so
   PIPE_SOURCES += pipe_r600.c
endif
endif

ifneq ($(findstring radeon/drm,$(GALLIUM_WINSYS_DIRS)),)
ifneq ($(findstring radeonsi,$(GALLIUM_DRIVERS_DIRS)),)
   _PIPE_TARGETS_CC += $(PIPE_PREFIX)radeonsi.so
   PIPE_SOURCES += pipe_radeonsi.c
endif
endif

ifneq ($(findstring svga/drm,$(GALLIUM_WINSYS_DIRS)),)
   _PIPE_TARGETS_CC += $(PIPE_PREFIX)vmwgfx.so
   PIPE_SOURCES += pipe_vmwgfx.c
endif

ifneq ($(filter llvmpipe softpipe,$(GALLIUM_DRIVERS_DIRS)),)
   _PIPE_TARGETS_CC += $(PIPE_PREFIX)swrast.so
   PIPE_SOURCES += pipe_swrast.c
endif

PIPE_OBJECTS := $(PIPE_SOURCES:.c=.o)

ifeq ($(MESA_LLVM),1)
   PIPE_TARGETS_CXX = $(_PIPE_TARGETS_CXX) $(_PIPE_TARGETS_CC)
   PIPE_TARGETS_CC =
else
   PIPE_TARGETS_CXX = $(_PIPE_TARGETS_CXX)
   PIPE_TARGETS_CC = $(_PIPE_TARGETS_CC)
endif

PIPE_TARGETS = $(PIPE_TARGETS_CC) $(PIPE_TARGETS_CXX)

default: depend $(PIPE_TARGETS)

.SECONDEXPANSION:

$(PIPE_TARGETS_CC): $(PIPE_PREFIX)%.so: pipe_%.o $(PIPE_LIBS) $$(%_LIBS)
	$(MKLIB) -o $@ -noprefix -linker '$(CC)' \
		-ldflags '-L$(TOP)/$(LIB_DIR) $(PIPE_LDFLAGS) $(LDFLAGS)' \
		$(MKLIB_OPTIONS) $< \
		-Wl,--start-group $(PIPE_LIBS) $($*_LIBS) -Wl,--end-group \
		$(PIPE_SYS) $($*_SYS)

$(PIPE_TARGETS_CXX): $(PIPE_PREFIX)%.so: pipe_%.o $(PIPE_LIBS) $$(%_LIBS)
	$(MKLIB) -o $@ -noprefix -linker '$(CXX)' \
		-ldflags '-L$(TOP)/$(LIB_DIR) $(PIPE_LDFLAGS) $(LDFLAGS)' \
		$(MKLIB_OPTIONS) $< \
		-Wl,--start-group $(PIPE_LIBS) $($*_LIBS) -Wl,--end-group \
		$(PIPE_SYS) $($*_SYS)

$(PIPE_OBJECTS): %.o: %.c
	$(CC) -c -o $@ $< $(PIPE_CPPFLAGS) $(PIPE_CFLAGS) $(CFLAGS)

install: $(PIPE_TARGETS)
	$(INSTALL) -d $(DESTDIR)/$(PIPE_INSTALL_DIR)
	for tgt in $(PIPE_TARGETS); do \
		$(MINSTALL) "$$tgt" $(DESTDIR)/$(PIPE_INSTALL_DIR); \
	done

clean:
	rm -f $(PIPE_TARGETS) $(PIPE_OBJECTS) depend depend.bak

depend: $(PIPE_SOURCES)
	rm -f depend
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(PIPE_CPPFLAGS) $(PIPE_SOURCES) 2>/dev/null

sinclude depend
