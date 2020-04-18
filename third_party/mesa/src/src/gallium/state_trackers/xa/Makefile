TOP = ../../../..
include $(TOP)/configs/current

##### MACROS #####

XA_CFLAGS = -Wall -pedantic

XA_INCLUDES= -I$(TOP)/src/gallium/ \
	-I$(TOP)/src/gallium/auxiliary \
	-I$(TOP)/src/gallium/include \
	-I$(TOP)/src/gallium/winsys \
	-I$(TOP)/src/gallium/drivers

XA_LIB = xatracker
XA_LIB_NAME = lib$(XA_LIB).o
XA_LIB_DEPS =

COMMON_GALLIUM_SOURCES=

SOURCES = \
	xa_tracker.c \
	xa_context.c \
	xa_renderer.c \
	xa_tgsi.c \
	xa_yuv.c \
	xa_composite.c
OBJECTS = $(SOURCES:.c=.o)

##### RULES #####

.c.o:
	$(CC) -c $(XA_CFLAGS) $(CFLAGS) $(XA_INCLUDES) $<


##### TARGETS #####

default: $(XA_LIB_NAME)


# Make the library
$(XA_LIB_NAME): depend $(OBJECTS)
	$(CC) -Wl,-r -nostdlib -o $(XA_LIB_NAME) $(OBJECTS) $(CFLAGS)

install: FORCE

clean:
	-rm -f *.o *~
	-rm -f *.lo
	-rm -f *.la
	-rm -f *.pc
	-rm -rf .libs
	-rm -f depend depend.bak


depend: $(SOURCES)
	@ echo "running $(MKDEP)"
	@ rm -f depend
	@ touch depend
	@ $(MKDEP) $(MKDEP_OPTIONS) -I$(TOP)/include $(XA_INCLUDES) $(SOURCES) \
		> /dev/null

-include depend

FORCE:
