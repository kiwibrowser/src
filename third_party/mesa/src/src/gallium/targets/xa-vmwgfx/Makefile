TOP = ../../../..
include $(TOP)/configs/current

##### MACROS #####

XA_MAJOR = 1
XA_MINOR = 0
XA_TINY = 0
XA_CFLAGS = -Wall -pedantic

XA_INCLUDES= -I$(TOP)/src/gallium/ \
	-I$(TOP)/src/gallium/auxiliary \
	-I$(TOP)/src/gallium/include \
	-I$(TOP)/src/gallium/winsys \
	-I$(TOP)/src/gallium/drivers

XA_LIB = xatracker
XA_LIB_NAME = lib$(XA_LIB).so
XA_LIB_GLOB = lib$(XA_LIB)*.so*
XA_LIB_DEPS = \
	$(TOP)/src/gallium/state_trackers/xa/libxatracker.o \
	$(TOP)/src/gallium/winsys/svga/drm/libsvgadrm.a \
	$(TOP)/src/gallium/drivers/svga/libsvga.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/rbug/librbug.a


COMMON_GALLIUM_SOURCES=

SOURCES = vmw_target.c
OBJECTS = $(SOURCES:.c=.o)

ifeq ($(MESA_LLVM),1)
LDFLAGS += $(LLVM_LDFLAGS)
GALLIUM_AUXILIARIES += $(LLVM_LIBS)
LD=$(CXX)
else
  ifeq ($(LINK_WITH_CXX),1)
    LD=$(CXX)
  else
    LD=$(CC)
  endif
endif

##### RULES #####

.c.o:
	$(CC) -c $(XA_CFLAGS) $(CFLAGS) $(XA_INCLUDES) $<


##### TARGETS #####

default: $(TOP)/$(LIB_DIR)/gallium/$(XA_LIB_NAME)

# Generate list of library visible symbols
xa_symbols: $(TOP)/src/gallium/state_trackers/xa/libxatracker.o
	$(NM) $< | $(GREP) ' T ' | $(AWK) '{print $$3}' > $@

# Make the library
$(TOP)/$(LIB_DIR)/gallium/$(XA_LIB_NAME): depend $(OBJECTS) $(XA_LIB_DEPS) \
	xa_symbols
	$(MKLIB) -o $(XA_LIB) -linker $(LD) -ldflags '$(LDFLAGS)' \
		-major $(XA_MAJOR) -minor $(XA_MINOR) -patch $(XA_TINY) \
		$(MKLIB_OPTIONS) \
		-exports xa_symbols \
		 -install $(TOP)/$(LIB_DIR)/gallium \
		$(OBJECTS) $(XA_LIB_DEPS) $(GALLIUM_AUXILIARIES)

# xa pkgconfig file
pcedit = sed \
	-e 's,@INSTALL_DIR@,$(INSTALL_DIR),g' \
	-e 's,@INSTALL_LIB_DIR@,$(INSTALL_LIB_DIR),g' \
	-e 's,@INSTALL_INC_DIR@,$(INSTALL_INC_DIR),g' \
	-e 's,@VERSION@,$(XA_MAJOR).$(XA_MINOR).$(XA_TINY),g' \
	-e 's,@XA_PC_REQ_PRIV@,$(XA_PC_REQ_PRIV),g' \
	-e 's,@XA_PC_LIB_PRIV@,$(XA_PC_LIB_PRIV),g' \
	-e 's,@XA_PC_CFLAGS@,$(XA_PC_CFLAGS),g' \
	-e 's,@XA_LIB@,$(XA_LIB),g'
xatracker.pc: xatracker.pc.in
	$(pcedit) $< > $@

install: xatracker.pc
	$(INSTALL) -d $(DESTDIR)$(INSTALL_INC_DIR)
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)/pkgconfig
	$(INSTALL) -m 644 $(TOP)/src/gallium/state_trackers/xa/xa_tracker.h $(DESTDIR)$(INSTALL_INC_DIR)
	$(INSTALL) -m 644 $(TOP)/src/gallium/state_trackers/xa/xa_context.h $(DESTDIR)$(INSTALL_INC_DIR)
	$(INSTALL) -m 644 $(TOP)/src/gallium/state_trackers/xa/xa_composite.h $(DESTDIR)$(INSTALL_INC_DIR)
	$(MINSTALL) -m 755 $(TOP)/$(LIB_DIR)/gallium/$(XA_LIB_GLOB) $(DESTDIR)$(INSTALL_LIB_DIR)
	$(INSTALL) -m 644 xatracker.pc $(DESTDIR)$(INSTALL_LIB_DIR)/pkgconfig

clean:
	-rm -f *.o *~
	-rm -f *.lo
	-rm -f *.la
	-rm -f *.pc
	-rm -rf .libs
	-rm -f depend depend.bak exptmp xa_symbols


depend: $(SOURCES)
	@ echo "running $(MKDEP)"
	@ rm -f depend
	@ touch depend
	@ $(MKDEP) $(MKDEP_OPTIONS) -I$(TOP)/include $(XA_INCLUDES) $(SOURCES) \
		> /dev/null

-include depend

FORCE:
