liblouisincludedir = $(includedir)/liblouis

liblouisinclude_HEADERS = \
	louis.h

# Don't include liblouis.h in dist, this will break subdir builds
# when dist is configured with a different ucs-option than the build,
# i.e. ucs2 dist and ucs4 build and vice versa.
nodist_liblouisinclude_HEADERS = \
	liblouis.h

lib_LTLIBRARIES = liblouis.la

AM_CPPFLAGS = -DTABLESDIR=\""$(datadir)"/liblouis/tables\"

liblouis_la_LDFLAGS =	\
	-version-info $(LIBLOUIS_CURRENT):$(LIBLOUIS_REVISION):$(LIBLOUIS_AGE) -no-undefined \
	$(LTLIBINTL)

liblouis_la_SOURCES = \
	lou_backTranslateString.c \
	compileTranslationTable.c \
	louis.h	\
	logging.c \
	lou_translateString.c \
	transcommon.ci \
	wrappers.c

# Don't include liblouis.h in dist, this will break subdir builds
# when dist is configured with a different ucs-option than the build,
# i.e. ucs2 dist and ucs4 build and vice versa.
nodist_liblouis_la_SOURCES = \
	liblouis.h
