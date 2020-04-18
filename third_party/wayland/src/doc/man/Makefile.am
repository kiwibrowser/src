#
# This generates man-pages out of the Docbook XML files. Simply add your files
# to the $MANPAGES array. If aliases are created, please add them to the
# MANPAGES_ALIASES array so they get installed correctly.
#

MANPAGES = \
	wl_display_connect.3
MANPAGES_ALIASES = \
	wl_display_connect_to_fd.3

XML_FILES = \
	${patsubst %.1,%.xml,${patsubst %.3,%.xml,${patsubst %.5,%.xml,${patsubst %.7,%.xml,$(MANPAGES)}}}}
CLEANFILES =
EXTRA_DIST = $(XML_FILES)

if HAVE_XSLTPROC
if HAVE_MANPAGES_STYLESHEET

CLEANFILES += $(MANPAGES) $(MANPAGES_ALIASES)
EXTRA_DIST += $(MANPAGES) $(MANPAGES_ALIASES)
dist_man_MANS = $(MANPAGES) $(MANPAGES_ALIASES)

XSLTPROC_FLAGS = \
	--stringparam man.authors.section.enabled 0 \
	--stringparam man.copyright.section.enabled 0 \
	--stringparam funcsynopsis.style ansi \
	--stringparam man.output.quietly 1 \
	--nonet

XSLTPROC_PROCESS_MAN = \
	$(AM_V_GEN)$(MKDIR_P) $(dir $@) && \
	$(XSLTPROC) -o $@ $(XSLTPROC_FLAGS) $(MANPAGES_STYLESHEET) $< && \
	$(SED) -i -e 's/^\.so \(.*\)\.\(.\)$$/\.so man\2\/\1\.\2/' $(MANPAGES_ALIASES)

%.1: %.xml
	$(XSLTPROC_PROCESS_MAN)

%.3: %.xml
	$(XSLTPROC_PROCESS_MAN)

%.5: %.xml
	$(XSLTPROC_PROCESS_MAN)

%.7: %.xml
	$(XSLTPROC_PROCESS_MAN)

wl_display_connect_to_fd.3: wl_display_connect.3

endif # HAVE_MANPAGES_STYLESHEET
endif # HAVE_XSLTPROC
