AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/gallium/include \
	-I$(top_srcdir)/src/gallium/auxiliary \
	-I$(top_srcdir)/src/gallium/drivers \
	$(DEFINES) \
	$(PIC_FLAGS)

AM_CFLAGS = $(VISIBILITY_CFLAGS)

noinst_LIBRARIES =

SUBDIRS =

################################################################################

if HAVE_GALAHAD_GALLIUM

noinst_LIBRARIES+= galahad/libgalahad.a

galahad_libgalahad_a_SOURCES = \
	galahad/glhd_objects.c \
	galahad/glhd_context.c \
	galahad/glhd_screen.c

endif

################################################################################

if HAVE_IDENTITY_GALLIUM

noinst_LIBRARIES+= identity/libidentity.a

identity_libidentity_a_SOURCES = \
	identity/id_objects.c \
	identity/id_context.c \
	identity/id_screen.c

endif

################################################################################

if HAVE_NOOP_GALLIUM

# Meta-driver which combines whichever software rasterizers have been
# built into a single convenience library.

noinst_LIBRARIES+= noop/libnoop.a

noop_libnoop_a_SOURCES = \
	noop/noop_pipe.c \
	noop/noop_state.c

endif

################################################################################

if NEED_RADEON_GALLIUM

SUBDIRS+= radeon

endif

################################################################################

SUBDIRS+= $(GALLIUM_MAKE_DIRS)

# FIXME: Remove when the rest of Gallium is converted to automake.
default: all

