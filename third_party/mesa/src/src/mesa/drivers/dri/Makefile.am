SUBDIRS =

if HAVE_COMMON_DRI
SUBDIRS+=common
endif

if HAVE_I915_DRI
SUBDIRS+=i915
endif

if HAVE_I965_DRI
SUBDIRS+=i965
endif

if HAVE_NOUVEAU_DRI
SUBDIRS+=nouveau
endif

if HAVE_R200_DRI
SUBDIRS+=r200
endif

if HAVE_RADEON_DRI
SUBDIRS+=radeon
endif

if HAVE_SWRAST_DRI
SUBDIRS+=swrast
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = dri.pc

driincludedir = $(includedir)/GL/internal
driinclude_HEADERS = $(top_srcdir)/include/GL/internal/dri_interface.h
