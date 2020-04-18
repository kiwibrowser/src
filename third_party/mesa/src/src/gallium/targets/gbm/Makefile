# src/gallium/targets/gbm/Makefile

TOP = ../../../..
include $(TOP)/configs/current

GBM_BACKEND = gbm_gallium_drm
GBM_SOURCES = gbm.c

GBM_INCLUDES = \
	       -I$(TOP)/include \
	       -I$(TOP)/src/gallium/state_trackers/gbm \
	       -I$(TOP)/src/gbm/main \
	       -I$(TOP)/src/gallium/auxiliary \
	       -I$(TOP)/src/gallium/winsys \
	       -I$(TOP)/src/gallium/include

GBM_LIBS = $(LIBUDEV_LIBS) $(LIBDRM_LIBS) -lm \
	   $(TOP)/src/gallium/state_trackers/gbm/libgbm.a \
	   $(GALLIUM_PIPE_LOADER_LIBS) $(GALLIUM_AUXILIARIES)

GBM_CFLAGS = \
	     -DPIPE_SEARCH_DIR=\"$(PIPE_INSTALL_DIR)\" \
             $(GALLIUM_PIPE_LOADER_DEFINES) \
	     $(LIBUDEV_CFLAGS) \
	     $(LIBDRM_CFLAGS)

GBM_EXTRA_TARGETS = pipes
GBM_EXTRA_INSTALL = install-pipes
GBM_EXTRA_CLEAN = clean-pipes

include $(TOP)/src/gbm/backends/Makefile.template

PIPE_SRC_DIR = $(TOP)/src/gallium/targets/pipe-loader
PIPE_INSTALL_DIR = $(INSTALL_LIB_DIR)/gbm

pipes:
	@$(MAKE) -C $(PIPE_SRC_DIR)
install-pipes:
	@$(MAKE) -C $(PIPE_SRC_DIR) PIPE_INSTALL_DIR=$(PIPE_INSTALL_DIR) install
clean-pipes:
	@$(MAKE) -C $(PIPE_SRC_DIR) clean
