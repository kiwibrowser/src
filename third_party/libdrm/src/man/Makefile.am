#
# This generates man-pages out of the Docbook XML files. Simply add your files
# to the relevant *man_PRE array. If aliases are created, please add them to the
# *man_aliases_PRE array so they get installed correctly.
#

libman_PRE = \
	drmAvailable.xml \
	drmHandleEvent.xml \
	drmModeGetResources.xml

miscman_PRE = \
	drm.xml \
	drm-kms.xml \
	drm-memory.xml

miscman_aliases_PRE = \
	drm-mm.xml \
	drm-gem.xml \
	drm-ttm.xml

libmandir = $(LIB_MAN_DIR)
miscmandir = $(MISC_MAN_DIR)
miscman_aliasesdir = $(MISC_MAN_DIR)

libman_DATA = $(libman_PRE:.xml=.$(LIB_MAN_SUFFIX))
miscman_DATA = $(miscman_PRE:.xml=.$(MISC_MAN_SUFFIX))
miscman_aliases_DATA = $(miscman_aliases_PRE:.xml=.$(MISC_MAN_SUFFIX))

XML_FILES = \
	$(libman_PRE) \
	$(miscman_PRE)

MAN_FILES = \
	$(libman_DATA) \
	$(miscman_DATA) \
	$(miscman_aliases_DATA)

EXTRA_DIST = $(XML_FILES)
CLEANFILES = $(MAN_FILES)

XSLTPROC_FLAGS = \
	--stringparam man.authors.section.enabled 0 \
	--stringparam man.copyright.section.enabled 0 \
	--stringparam funcsynopsis.style ansi \
	--stringparam man.output.quietly 1 \
	--nonet \
	$(MANPAGES_STYLESHEET)

XSLTPROC_PROCESS_MAN = \
	$(AM_V_GEN)$(XSLTPROC) -o "$@" $(XSLTPROC_FLAGS) "$<"

$(miscman_aliases_DATA): $(miscman_DATA)
	$(AM_V_GEN)if test -n "$@" ; then $(SED) -i -e 's/^\.so \([a-z_]\+\)\.\([0-9]\)$$/\.so man\2\/\1\.\2/' "$@" ; fi

SUFFIXES = .$(LIB_MAN_SUFFIX) .$(MISC_MAN_SUFFIX) .xml

.xml.$(LIB_MAN_SUFFIX):
	$(XSLTPROC_PROCESS_MAN)

.xml.$(MISC_MAN_SUFFIX):
	$(XSLTPROC_PROCESS_MAN)
