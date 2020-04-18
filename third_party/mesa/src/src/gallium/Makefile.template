# src/gallium/Makefile.template

# Template makefile for gallium libraries.
#
# Usage:
#   The minimum that the including makefile needs to define
#   is TOP, LIBNAME and one of of the *_SOURCES.
#
# Optional defines:
#   LIBRARY_INCLUDES are appended to the list of includes directories.
#   LIBRARY_DEFINES is not used for makedepend, but for compilation.

### Basic defines ###

OBJECTS = $(C_SOURCES:.c=.o) \
	$(CPP_SOURCES:.cpp=.o) \
	$(ASM_SOURCES:.S=.o)

INCLUDES = \
	-I. \
	-I$(TOP)/src/gallium/include \
	-I$(TOP)/src/gallium/auxiliary \
	-I$(TOP)/src/gallium/drivers \
	$(LIBRARY_INCLUDES)

ifeq ($(MESA_LLVM),1)
LIBRARY_DEFINES += $(LLVM_CFLAGS)
endif


##### TARGETS #####

default: depend lib$(LIBNAME).a $(PROGS)

lib$(LIBNAME).a: $(OBJECTS) $(EXTRA_OBJECTS) Makefile $(TOP)/src/gallium/Makefile.template
	$(MKLIB) -o $(LIBNAME) -static $(OBJECTS) $(EXTRA_OBJECTS)

depend: $(C_SOURCES) $(CPP_SOURCES) $(ASM_SOURCES) $(SYMLINKS) $(GENERATED_SOURCES)
	rm -f depend
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(INCLUDES) $(C_SOURCES) $(CPP_SOURCES) $(ASM_SOURCES) $(GENERATED_SOURCES) 2> /dev/null

$(PROGS): % : %.o $(PROGS_DEPS)
	$(LD) $(LDFLAGS) $(filter %.o,$^) -o $@ -Wl,--start-group  $(LIBS) -Wl,--end-group

# Emacs tags
tags:
	etags `find . -name \*.[ch]` `find $(TOP)/src/gallium/include -name \*.h`

# Remove .o and backup files
clean:
	rm -f $(OBJECTS) $(GENERATED_SOURCES) $(PROGS) lib$(LIBNAME).a depend depend.bak $(CLEAN_EXTRA)

# Dummy target
install:
	@echo -n ""

##### RULES #####

%.s: %.c
	$(CC) -S $(INCLUDES) $(CFLAGS) $(LIBRARY_DEFINES) $< -o $@

%.o: %.c
	$(CC) -c $(INCLUDES) $(CFLAGS) $(LIBRARY_DEFINES) $< -o $@

%.o: %.cpp
	$(CXX) -c $(INCLUDES) $(CXXFLAGS) $(LIBRARY_DEFINES) $< -o $@

%.o: %.S
	$(CC) -c $(INCLUDES) $(CFLAGS) $(LIBRARY_DEFINES)  $< -o $@


sinclude depend
