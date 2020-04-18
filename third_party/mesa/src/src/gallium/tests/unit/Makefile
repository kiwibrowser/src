# progs/gallium/simple/Makefile

TOP = ../../../..
include $(TOP)/configs/current

INCLUDES = \
	-I. \
	-I$(TOP)/src/gallium/include \
	-I$(TOP)/src/gallium/auxiliary \
	-I$(TOP)/src/gallium/drivers \
	-I$(TOP)/src/gallium/winsys \
	$(PROG_INCLUDES)

LINKS = \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/winsys/sw/null/libws_null.a \
	$(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a \
	$(GALLIUM_AUXILIARIES) \
	$(PROG_LINKS)

SOURCES = \
	pipe_barrier_test.c \
	u_cache_test.c \
	u_half_test.c \
	u_format_test.c \
	u_format_compatible_test.c \
	translate_test.c


OBJECTS = $(SOURCES:.c=.o)

PROGS = $(OBJECTS:.o=)

##### TARGETS #####

default: $(PROGS)

clean:
	-rm -f $(PROGS)
	-rm -f *.o
	-rm -f result.bmp

##### RULES #####

$(OBJECTS): %.o: %.c
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $(PROG_DEFINES) $< -o $@

$(PROGS): %: %.o
	$(CC) $(LDFLAGS) $< $(LINKS) -lm -lpthread -ldl -o $@
