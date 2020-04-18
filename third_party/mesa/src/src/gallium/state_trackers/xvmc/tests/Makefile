TOP = ../../../../..
include $(TOP)/configs/current

LIBS = -lXvMCW -lXvMC -lXv -lX11

#############################################

.PHONY: default clean

default: test_context test_surface test_subpicture test_blocks test_rendering xvmc_bench

test_context: test_context.o testlib.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

test_surface: test_surface.o testlib.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

test_subpicture: test_subpicture.o testlib.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

test_blocks: test_blocks.o testlib.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

test_rendering: test_rendering.o testlib.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

xvmc_bench: xvmc_bench.o testlib.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	$(RM) -rf *.o test_context test_surface test_subpicture test_blocks test_rendering xvmc_bench
