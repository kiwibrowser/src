# src/gallium/targets/libgl-xlib/Makefile

# This makefile produces a "stand-alone" libGL.so which is based on
# Xlib (no DRI HW acceleration)


TOP = ../../../..
include $(TOP)/configs/current


GL_MAJOR = 1
GL_MINOR = 5
GL_TINY = 0$(MESA_MAJOR)$(MESA_MINOR)0$(MESA_TINY)


INCLUDE_DIRS = \
	-I$(TOP)/include \
	-I$(TOP)/src/mapi \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/mesa/main \
	-I$(TOP)/src/gallium/include \
	-I$(TOP)/src/gallium/drivers \
	-I$(TOP)/src/gallium/state_trackers/glx/xlib \
	-I$(TOP)/src/gallium/auxiliary \
	$(X11_CFLAGS)

DEFINES += \
	-DGALLIUM_SOFTPIPE \
	-DGALLIUM_RBUG \
	-DGALLIUM_TRACE \
	-DGALLIUM_GALAHAD

XLIB_TARGET_SOURCES = \
	xlib.c


XLIB_TARGET_OBJECTS = $(XLIB_TARGET_SOURCES:.c=.o)



LIBS = \
	$(GALLIUM_DRIVERS) \
	$(TOP)/src/gallium/state_trackers/glx/xlib/libxlib.a \
	$(TOP)/src/gallium/winsys/sw/xlib/libws_xlib.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/rbug/librbug.a \
	$(TOP)/src/gallium/drivers/galahad/libgalahad.a \
	$(TOP)/src/mapi/glapi/libglapi.a \
	$(TOP)/src/mesa/libmesagallium.a \
	$(GALLIUM_AUXILIARIES) \


# LLVM
ifeq ($(MESA_LLVM),1)
PIPE_DRIVERS += $(TOP)/src/gallium/drivers/llvmpipe/libllvmpipe.a
DEFINES += -DGALLIUM_LLVMPIPE
GL_LIB_DEPS += $(LLVM_LIBS) 
LDFLAGS += $(LLVM_LDFLAGS)
endif

ifeq ($(SHARED_GLAPI),1)
GL_LIB_DEPS := -L$(TOP)/$(LIB_DIR) -l$(GLAPI_LIB) $(GL_LIB_DEPS)
endif


.SUFFIXES : .cpp

.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@

.cpp.o:
	$(CXX) -c $(INCLUDE_DIRS) $(CXXFLAGS) $< -o $@



default: $(TOP)/$(LIB_DIR)/gallium $(TOP)/$(LIB_DIR)/gallium/$(GL_LIB_NAME)

$(TOP)/$(LIB_DIR)/gallium:
	@ mkdir -p $(TOP)/$(LIB_DIR)/gallium

# Make the libGL.so library
$(TOP)/$(LIB_DIR)/gallium/$(GL_LIB_NAME): $(XLIB_TARGET_OBJECTS) $(LIBS) Makefile
	$(TOP)/bin/mklib -o $(GL_LIB) \
		-linker "$(CXX)" -ldflags '$(LDFLAGS)' \
		-major $(GL_MAJOR) -minor $(GL_MINOR) -patch $(GL_TINY) \
		-cplusplus \
		-install $(TOP)/$(LIB_DIR)/gallium \
		$(MKLIB_OPTIONS) $(XLIB_TARGET_OBJECTS) \
		-Wl,--start-group $(LIBS) -Wl,--end-group $(GL_LIB_DEPS)


depend: $(XLIB_TARGET_SOURCES)
	@ echo "running $(MKDEP)"
	@ rm -f depend  # workaround oops on gutsy?!?
	@ touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(DEFINES) $(INCLUDE_DIRS) $(XLIB_TARGET_SOURCES) \
		> /dev/null 2>/dev/null


install: default
	$(INSTALL) -d $(DESTDIR)$(INSTALL_DIR)/include/GL
	$(INSTALL) -d $(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR)
	$(INSTALL) -m 644 $(TOP)/include/GL/*.h $(DESTDIR)$(INSTALL_DIR)/include/GL
	@if [ -e $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME) ]; then \
		$(MINSTALL) $(TOP)/$(LIB_DIR)/libGL* $(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR); \
	fi


# Emacs tags
tags:
	etags `find . -name \*.[ch]` $(TOP)/include/GL/*.h

clean:
	-rm -f *.o depend


include depend
