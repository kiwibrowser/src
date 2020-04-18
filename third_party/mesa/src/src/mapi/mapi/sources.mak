# src/mapi/mapi/sources.mak
#
# mapi may be used in several ways
#
#  - In default mode, mapi implements the interface defined by mapi.h.  To use
#    this mode, compile MAPI_FILES.
#
#  - In util mode, mapi provides utility functions for use with glapi.  To use
#    this mode, compile MAPI_UTIL_FILES with MAPI_MODE_UTIL defined.
#
#  - In glapi mode, mapi implements the interface defined by glapi.h.  To use
#    this mode, compile MAPI_GLAPI_FILES with MAPI_MODE_GLAPI defined.
#
#  - In bridge mode, mapi provides entry points calling into glapi.  To use
#    this mode, compile MAPI_BRIDGE_FILES with MAPI_MODE_BRIDGE defined.

MAPI_UTIL_FILES = \
	$(TOP)/src/mapi/mapi/u_current.c \
	$(TOP)/src/mapi/mapi/u_execmem.c

MAPI_FILES = \
	$(TOP)/src/mapi/mapi/entry.c \
	$(TOP)/src/mapi/mapi/mapi.c \
	$(TOP)/src/mapi/mapi/stub.c \
	$(TOP)/src/mapi/mapi/table.c \
	$(MAPI_UTIL_FILES)

MAPI_GLAPI_FILES = \
	$(TOP)/src/mapi/mapi/entry.c \
	$(TOP)/src/mapi/mapi/mapi_glapi.c \
	$(TOP)/src/mapi/mapi/stub.c \
	$(TOP)/src/mapi/mapi/table.c \
	$(MAPI_UTIL_FILES)

MAPI_BRIDGE_FILES = \
	$(TOP)/src/mapi/mapi/entry.c
