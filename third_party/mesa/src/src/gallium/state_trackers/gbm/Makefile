TOP = ../../../..
include $(TOP)/configs/current

gbm_INCLUDES = \
	-I. \
	-I$(TOP)/src/gallium/include \
	-I$(TOP)/src/gallium/auxiliary \
	-I$(TOP)/src/gbm/main \
	-I$(TOP)/include

gbm_SOURCES = $(wildcard *.c)
gbm_OBJECTS = $(gbm_SOURCES:.c=.o)

ALL_INCLUDES = $(gbm_INCLUDES)
ALL_SOURCES = $(gbm_SOURCES)

GBM_OBJECTS = $(gbm_OBJECTS)
GBM_CPPFLAGS = $(gbm_INCLUDES)

ifneq ($(findstring wayland, $(EGL_PLATFORMS)),)
GBM_CPPFLAGS += -DHAVE_WAYLAND_PLATFORM
endif

##### TARGETS #####

default: depend libgbm.a

libgbm.a: $(GBM_OBJECTS) Makefile
	$(MKLIB) -o gbm -static $(GBM_OBJECTS)

depend: 
	rm -f depend
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(ALL_INCLUDES) $(ALL_SOURCES) 2> /dev/null

clean:
	rm -f libgbm.a
	rm -f $(GBM_OBJECTS)
	rm -f depend depend.bak

# Dummy target
install:
	@echo -n ""

##### RULES #####

$(gbm_OBJECTS): %.o: %.c
	$(CC) -c $(GBM_CPPFLAGS) $(DEFINES) $(CFLAGS) $< -o $@

sinclude depend
