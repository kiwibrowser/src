CFLAGS = -Wall --std=c99
# This is where `make install` from libelf places its stuff
LIBELF_HEADERS = /usr/local/include/libelf/
LIBELF_LIBS = /usr/local/lib/

all: elf_creator

elf_creator: elf_creator.c
	gcc $(CFLAGS) -o elf_creator elf_creator.c \
		-I$(LIBELF_HEADERS) -L$(LIBELF_LIBS) -Wl,-rpath,$(LIBELF_LIBS) -lelf

clean:
	rm -f *.o
	rm -f *.so
	rm -f *.a
	rm -f elf_creator


