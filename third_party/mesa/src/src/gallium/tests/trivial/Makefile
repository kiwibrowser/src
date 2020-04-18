# src/gallium/tests/trivial/Makefile

TOP = ../../../..
include $(TOP)/configs/current

INCLUDES = \
	-I. \
	-I$(TOP)/src/gallium/include \
	-I$(TOP)/src/gallium/auxiliary \
	-I$(TOP)/src/gallium/drivers \
	-I$(TOP)/src/gallium/winsys \
	$(PROG_INCLUDES)

LINKS += \
	$(GALLIUM_PIPE_LOADER_LIBS) \
	$(GALLIUM_AUXILIARIES) \
	$(PROG_LINKS) $(LIBUDEV_LIBS)

SOURCES = \
	tri.c \
	quad-tex.c \
	compute.c

OBJECTS = $(SOURCES:.c=.o)

PROGS = $(OBJECTS:.o=)

PROG_DEFINES = -DPIPE_SEARCH_DIR=\"$(PIPE_SRC_DIR)\" \
               $(GALLIUM_PIPE_LOADER_DEFINES)

PIPE_SRC_DIR = $(TOP)/src/gallium/targets/pipe-loader

##### TARGETS #####

default: $(PROGS) pipes

install:

clean:
	-rm -f $(PROGS)
	-rm -f *.o
	-rm -f result.bmp
	@$(MAKE) -C $(PIPE_SRC_DIR) clean

pipes:
	@$(MAKE) -C $(PIPE_SRC_DIR)

##### RULES #####

$(OBJECTS): %.o: %.c
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $(PROG_DEFINES) $< -o $@

$(PROGS): %: %.o $(LINKS)
	$(CXX) $(LDFLAGS) $< $(LINKS) $(LLVM_LIBS) -lm -lpthread -ldl -o $@
