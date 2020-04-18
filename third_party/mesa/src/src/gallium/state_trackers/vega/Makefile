# src/gallium/state_trackers/vega/Makefile

TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = vega

LIBRARY_INCLUDES = \
	-I$(TOP)/include \
	-I$(TOP)/src/mapi

LIBRARY_DEFINES = \
	-DVEGA_VERSION_STRING=\"$(MESA_VERSION)\"

C_SOURCES = \
           api.c           \
           api_context.c   \
           api_filters.c   \
           api_images.c    \
           api_masks.c     \
           api_misc.c      \
           api_paint.c     \
           api_params.c    \
           api_path.c      \
           api_text.c      \
           api_transform.c \
           arc.c \
           bezier.c \
           handle.c \
           image.c \
           mask.c \
           paint.c \
           path.c \
           polygon.c \
           renderer.c \
           shader.c \
           shaders_cache.c \
           stroker.c \
           text.c \
           vg_context.c \
           vg_manager.c \
           vg_state.c \
           vg_translate.c \
           vgu.c


GENERATED_SOURCES := api_tmp.h

include ../../Makefile.template

MAPI := $(TOP)/src/mapi
api_tmp.h: $(MAPI)/mapi/mapi_abi.py $(MAPI)/vgapi/vgapi.csv
	$(PYTHON2) $< --printer vgapi --mode app $(MAPI)/vgapi/vgapi.csv > $@
