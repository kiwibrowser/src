# src/gallium/targets/egl-static/Makefile
#
# This is Makefile for egl_gallium.so.  It is static in that all state trackers
# and pipe drivers are linked statically when possible.
#
# The following variables are examined
#
#   EGL_PLATFORMS       - platforms to support
#   EGL_CLIENT_APIS     - state trackers to support
#   GALLIUM_WINSYS_DIRS - pipe drivers to support
#   SHARED_GLAPI        - st/mesa can be statically linked or not
#

TOP = ../../../..
include $(TOP)/configs/current

OUTPUTS := egl_gallium

egl_CPPFLAGS := \
	-I$(TOP)/include \
	-I$(TOP)/src/gallium/auxiliary \
	-I$(TOP)/src/gallium/drivers \
	-I$(TOP)/src/gallium/include \
	-I$(TOP)/src/gallium/winsys
egl_LIBS := \
	$(TOP)/src/gallium/drivers/identity/libidentity.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/rbug/librbug.a \
	$(GALLIUM_AUXILIARIES)
egl_SYS :=

egl_SOURCES := \
	egl.c \
	egl_pipe.c \
	egl_st.c

egl_OBJECTS := $(egl_SOURCES:%.c=%.o)

# st/egl
egl_CPPFLAGS += \
	-I$(TOP)/src/gallium/state_trackers/egl \
	-I$(TOP)/src/egl/main \
	-D_EGL_MAIN=_eglMain
egl_LIBS += $(TOP)/src/gallium/state_trackers/egl/libegl.a
egl_SYS += $(LIBUDEV_LIBS) $(DLOPEN_LIBS) -lEGL -lm -lpthread

# EGL platforms
ifneq ($(findstring x11, $(EGL_PLATFORMS)),)
egl_CPPFLAGS += $(LIBDRM_CFLAGS)
egl_LIBS += $(TOP)/src/gallium/winsys/sw/xlib/libws_xlib.a
egl_SYS += -lX11 -lXext -lXfixes $(LIBDRM_LIBS)
endif
ifneq ($(findstring wayland, $(EGL_PLATFORMS)),)
egl_CPPFLAGS += $(LIBDRM_CFLAGS)
egl_LIBS += $(TOP)/src/gallium/winsys/sw/wayland/libws_wayland.a
egl_LIBS += $(TOP)/src/egl/wayland/wayland-drm/.libs/libwayland-drm.a
egl_SYS += $(LIBDRM_LIBS) $(WAYLAND_LIBS)
endif
ifneq ($(findstring drm, $(EGL_PLATFORMS)),)
egl_CPPFLAGS += $(LIBDRM_CFLAGS)
egl_SYS += $(LIBDRM_LIBS) -lgbm
endif
ifneq ($(findstring fbdev, $(EGL_PLATFORMS)),)
egl_LIBS += $(TOP)/src/gallium/winsys/sw/fbdev/libfbdev.a
endif
ifneq ($(findstring null, $(EGL_PLATFORMS)),)
egl_LIBS += $(TOP)/src/gallium/winsys/sw/null/libws_null.a
endif

# st/mesa
ifneq ($(filter $(GL_LIB), $(EGL_CLIENT_APIS)),)
egl_CPPFLAGS += -I$(TOP)/src/mesa $(API_DEFINES)
# make st/mesa built-in when there is a single glapi provider
ifeq ($(SHARED_GLAPI),1)
egl_LIBS += $(TOP)/src/mesa/libmesagallium.a
egl_SYS += -lm -lpthread $(DLOPEN_LIBS) -l$(GLAPI_LIB)
else
egl_CPPFLAGS += -D_EGL_EXTERNAL_GL=1
OUTPUTS += st_GL
endif # SHARED_GLAPI
endif

# st/vega
ifneq ($(filter $(VG_LIB), $(EGL_CLIENT_APIS)),)
egl_CPPFLAGS += -I$(TOP)/src/gallium/state_trackers/vega -DFEATURE_VG=1
egl_LIBS += $(TOP)/src/gallium/state_trackers/vega/libvega.a
egl_SYS += -lm -l$(VG_LIB)
endif

# i915
ifneq ($(findstring i915/drm,$(GALLIUM_WINSYS_DIRS)),)
egl_CPPFLAGS += -D_EGL_PIPE_I915=1
egl_LIBS += \
	$(TOP)/src/gallium/winsys/i915/drm/libi915drm.a \
	$(TOP)/src/gallium/drivers/i915/libi915.a
egl_SYS += $(INTEL_LIBS)
endif

# nouveau
ifneq ($(findstring nouveau/drm,$(GALLIUM_WINSYS_DIRS)),)
egl_CPPFLAGS += -D_EGL_PIPE_NOUVEAU=1
egl_LIBS += \
	$(TOP)/src/gallium/winsys/nouveau/drm/libnouveaudrm.a \
	$(TOP)/src/gallium/drivers/nv30/libnv30.a \
	$(TOP)/src/gallium/drivers/nv50/libnv50.a \
	$(TOP)/src/gallium/drivers/nvc0/libnvc0.a \
	$(TOP)/src/gallium/drivers/nouveau/libnouveau.a
egl_SYS += $(NOUVEAU_LIBS)
endif

# r300
ifneq ($(findstring radeon/drm,$(GALLIUM_WINSYS_DIRS)),)
ifneq ($(findstring r300,$(GALLIUM_DRIVERS_DIRS)),)
egl_CPPFLAGS += -D_EGL_PIPE_R300=1
egl_LIBS += \
	$(TOP)/src/gallium/winsys/radeon/drm/libradeonwinsys.a \
	$(TOP)/src/gallium/drivers/r300/libr300.a
egl_SYS += $(RADEON_LIBS)
endif
endif

# r600
ifneq ($(findstring radeon/drm,$(GALLIUM_WINSYS_DIRS)),)
ifneq ($(findstring r600,$(GALLIUM_DRIVERS_DIRS)),)
egl_CPPFLAGS += -D_EGL_PIPE_R600=1
egl_LIBS += \
	$(TOP)/src/gallium/winsys/radeon/drm/libradeonwinsys.a \
	$(TOP)/src/gallium/drivers/r600/libr600.a
egl_SYS += $(RADEON_LIBS)
endif
endif

# radeonsi
ifneq ($(findstring radeon/drm,$(GALLIUM_WINSYS_DIRS)),)
ifneq ($(findstring radeonsi,$(GALLIUM_DRIVERS_DIRS)),)
egl_CPPFLAGS += -D_EGL_PIPE_RADEONSI=1
egl_LIBS += \
	$(TOP)/src/gallium/winsys/radeon/drm/libradeonwinsys.a \
	$(TOP)/src/gallium/drivers/radeonsi/libradeonsi.a
egl_SYS += $(RADEON_LIBS)
endif
endif

# vmwgfx
ifneq ($(findstring svga/drm,$(GALLIUM_WINSYS_DIRS)),)
egl_CPPFLAGS += -D_EGL_PIPE_VMWGFX=1
egl_LIBS += \
	$(TOP)/src/gallium/winsys/svga/drm/libsvgadrm.a \
	$(TOP)/src/gallium/drivers/svga/libsvga.a
endif

# softpipe
ifneq ($(findstring softpipe,$(GALLIUM_DRIVERS_DIRS)),)
egl_CPPFLAGS += -DGALLIUM_SOFTPIPE -DGALLIUM_RBUG -DGALLIUM_TRACE
egl_LIBS += $(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a
egl_SYS += -lm
endif

# llvmpipe
ifneq ($(findstring llvmpipe,$(GALLIUM_DRIVERS_DIRS)),)
egl_CPPFLAGS += -DGALLIUM_LLVMPIPE
egl_LIBS += $(TOP)/src/gallium/drivers/llvmpipe/libllvmpipe.a
endif

# sort to remove duplicates
egl_CPPFLAGS := $(sort $(egl_CPPFLAGS))
egl_LIBS := $(sort $(egl_LIBS))
egl_SYS := $(sort $(egl_SYS))

# st_GL, built only when shared glapi is not enabled
st_GL_CPPFLAGS := -I $(TOP)/src/mesa -I$(TOP)/src/gallium/include
st_GL_LIBS := $(TOP)/src/mesa/libmesagallium.a $(GALLIUM_AUXILIARIES)
st_GL_SYS := -lm -lpthread $(DLOPEN_LIBS)

# LLVM
ifeq ($(MESA_LLVM),1)
egl_SYS += $(LLVM_LIBS)
LDFLAGS += $(LLVM_LDFLAGS)

st_GL_SYS += $(LLVM_LIBS)
endif

OUTPUT_PATH := $(TOP)/$(LIB_DIR)/egl
OUTPUTS := $(addprefix $(OUTPUT_PATH)/, $(addsuffix .so, $(OUTPUTS)))

default: $(OUTPUTS)

$(OUTPUT_PATH)/egl_gallium.so: $(egl_OBJECTS) $(egl_LIBS)
	$(MKLIB) -o $(notdir $@) -noprefix -linker '$(CXX)' \
		-ldflags '-L$(TOP)/$(LIB_DIR) -Wl,--no-undefined $(LDFLAGS)' \
		-cplusplus -install $(OUTPUT_PATH) $(MKLIB_OPTIONS) \
		$(egl_OBJECTS) -Wl,--start-group $(egl_LIBS) -Wl,--end-group \
		$(egl_SYS)

$(OUTPUT_PATH)/st_GL.so: st_GL.o $(st_GL_LIBS)
	$(MKLIB) -o $(notdir $@) -noprefix -linker '$(CXX)' \
		-ldflags '-L$(TOP)/$(LIB_DIR) $(LDFLAGS)' \
		-cplusplus -install $(OUTPUT_PATH) $(MKLIB_OPTIONS) \
		$< -Wl,--start-group $(st_GL_LIBS) -Wl,--end-group \
		$(st_GL_SYS)

$(egl_OBJECTS): %.o: %.c
	$(CC) -c -o $@ $< $(egl_CPPFLAGS) $(DEFINES) $(CFLAGS)

st_GL.o: st_GL.c
	$(CC) -c -o $@ $< $(st_GL_CPPFLAGS) $(DEFINES) $(CFLAGS)

install: $(OUTPUTS)
	$(INSTALL) -d $(DESTDIR)$(EGL_DRIVER_INSTALL_DIR)
	for out in $(OUTPUTS); do \
		$(MINSTALL) -m 755 "$$out" $(DESTDIR)$(EGL_DRIVER_INSTALL_DIR); \
	done

clean:
	rm -f *.o
