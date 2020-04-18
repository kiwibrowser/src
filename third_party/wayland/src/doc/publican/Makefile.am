# Documentation is built with xmlto, but some of the recipes in here are
# leftovers from building with Publican (https://fedorahosted.org/publican/)
#
# How this build works:
# * the main target is Wayland, documentation ends up in $(builddir)/Wayland/
# * hand-written chapters and CSS files are located in sources. These are
#   copied into $(builddir)/en-US/
# * ProtocolSpec.xml is generated from $(top_srcdir)/protocol/wayland.xml,
#   changed into docbook via XSLT and saved in $(builddir)/en-US/
# * ProtocolInterfaces.xml, same as above, uses a different XSLT
# * *API.xml is generated from the doxygen output and saved in
#   $(builddir)/en-US
# * run xmlto on $(builddir)/en-US, output to $(builddir)/Wayland/en-US

doxydir := $(top_builddir)/doc/doxygen
html_destdir := $(builddir)/Wayland/en-US/html

publican_sources = \
	$(srcdir)/sources/Wayland.ent \
	$(srcdir)/sources/Wayland.xml \
	$(srcdir)/sources/Book_Info.xml \
	$(srcdir)/sources/Author_Group.xml \
	$(srcdir)/sources/Foreword.xml \
	$(srcdir)/sources/Preface.xml \
	$(srcdir)/sources/Revision_History.xml \
	$(srcdir)/sources/Protocol.xml \
	$(srcdir)/sources/Compositors.xml \
	$(srcdir)/sources/images/icon.svg  \
	$(srcdir)/sources/images/wayland.png \
	$(srcdir)/sources/Client.xml \
	$(srcdir)/sources/Server.xml

processed_sources := \
	$(srcdir)/sources/Architecture.xml \
	$(srcdir)/sources/Introduction.xml

css_sources = \
	$(srcdir)/sources/css/brand.css \
	$(srcdir)/sources/css/common.css \
	$(srcdir)/sources/css/default.css \
	$(srcdir)/sources/css/epub.css \
	$(srcdir)/sources/css/print.css

img_sources = \
	$(srcdir)/sources/images/icon.svg \
	$(srcdir)/sources/images/wayland.png

doxygen_img_sources := \
	$(doxydir)/xml/wayland-architecture.png \
	$(doxydir)/xml/x-architecture.png

map_sources := \
	$(doxydir)/xml/x-architecture.map \
	$(doxydir)/xml/wayland-architecture.map

if HAVE_XMLTO
if HAVE_XSLTPROC
noinst_DATA = $(builddir)/Wayland $(publican_targets)
XMLTO_PARAM = \
	--skip-validation \
	--stringparam chunk.section.depth=0 \
	--stringparam toc.section.depth=1 \
	--stringparam html.stylesheet=css/default.css

# Listing various directories that might need to be created.
alldirs := $(builddir)/en-US $(builddir)/en-US/images $(html_destdir) $(html_destdir)/css $(html_destdir)/images


html_css_targets = $(addprefix $(html_destdir)/css/,$(notdir $(css_sources)))
html_img_targets = $(addprefix $(html_destdir)/images/,$(notdir $(img_sources)))
doxygen_img_targets := $(doxygen_img_sources:$(doxydir)/xml/%=$(html_destdir)/images/%)
map_targets := $(map_sources:$(doxydir)/xml/%=$(builddir)/en-US/images/%)
processed_targets := $(processed_sources:$(srcdir)/sources/%=$(builddir)/en-US/%)

$(builddir)/Wayland: $(publican_targets) $(html_css_targets) $(html_img_targets) $(processed_targets) $(doxygen_img_targets) | $(builddir)/en-US
	$(AM_V_GEN)$(XMLTO) $(XMLTO_PARAM) html $(builddir)/en-US/Wayland.xml -o $(html_destdir)
	@touch $@

$(html_destdir)/css/%: $(srcdir)/sources/css/% | $(html_destdir)/css
	$(AM_V_GEN)cp -f $< $@

$(html_destdir)/images/%: $(srcdir)/sources/images/% | $(html_destdir)/images
	$(AM_V_GEN)cp -f $< $@

$(html_destdir)/images/%: $(doxydir)/xml/% | $(html_destdir)/images
	$(AM_V_GEN)cp -f $< $@

pubdir = $(docdir)/Wayland/en-US

publican_targets = $(publican_sources:$(srcdir)/sources/%=$(builddir)/en-US/%) \
	$(builddir)/en-US/ProtocolSpec.xml \
	$(builddir)/en-US/ProtocolInterfaces.xml \
	$(builddir)/en-US/ClientAPI.xml \
	$(builddir)/en-US/ServerAPI.xml

# The Protocol.xml is purely generated and required before running publican
$(builddir)/en-US/ProtocolSpec.xml: $(top_srcdir)/protocol/wayland.xml $(srcdir)/protocol-to-docbook.xsl | $(builddir)/en-US
	$(AM_V_GEN)$(XSLTPROC) $(srcdir)/protocol-to-docbook.xsl \
		$(top_srcdir)/protocol/wayland.xml > $@

$(builddir)/en-US/ProtocolInterfaces.xml: $(top_srcdir)/protocol/wayland.xml $(srcdir)/protocol-interfaces-to-docbook.xsl | $(builddir)/en-US
	$(AM_V_GEN)$(XSLTPROC) $(srcdir)/protocol-interfaces-to-docbook.xsl \
		$(top_srcdir)/protocol/wayland.xml > $@

# * use doxygen's combine.xslt to merge the xml files into one single file
# * pipe that through the doxygen-to-publican stylesheet
$(builddir)/en-US/%API.xml: $(doxydir)/xml/%/index.xml $(srcdir)/doxygen-to-publican.xsl | $(builddir)/en-US
	$(AM_V_GEN)$(XSLTPROC) $(doxydir)/xml/$*/combine.xslt \
		$(doxydir)/xml/$*/index.xml | \
		$(XSLTPROC) --stringparam which $* \
		$(srcdir)/doxygen-to-publican.xsl - > $@

# Copy the sources source files into en-US destination
# This is required for out-of-source-tree build as publican does not allow us
# to specify the location of the source code.
$(builddir)/en-US/%: $(srcdir)/sources/% $(publican_sources) | $(builddir)/en-US/images
	$(AM_V_GEN)cp -f $< $@
	$(AM_V_at)chmod a+w $@

$(builddir)/en-US/images/%: $(doxydir)/xml/% | $(builddir)/en-US/images
	$(AM_V_GEN)cp -f $< $@
	$(AM_V_at)chmod a+w $@

# More specific rule to override explicitly listed targets and perform xslt
# modifications on them.
# Note that we can't use $< as all targets must be there
$(processed_targets): $(processed_sources) $(map_targets) $(srcdir)/merge-mapcoords.xsl | $(builddir)/en-US/images
	$(AM_V_GEN)$(XSLTPROC) --stringparam basedir $(builddir)/en-US \
		$(srcdir)/merge-mapcoords.xsl $(addprefix $(srcdir)/sources/,$(notdir $@)) > $@

# general rule to create one of the listed directories.
$(alldirs):
	$(AM_V_GEN)$(MKDIR_P) $@

CLEANFILES = $(publican_targets)

clean-local:
	$(AM_V_at)rm -fr $(builddir)/en-US
	$(AM_V_at)rm -fr $(builddir)/Wayland

install-data-local:
	test -z "$(pubdir)/html/css" || $(mkdir_p) "$(DESTDIR)$(pubdir)/html/css"
	test -z "$(pubdir)/html/images" || $(mkdir_p) "$(DESTDIR)$(pubdir)/html/images"
	list=`find $(builddir)/Wayland/en-US -type f`; \
	for p in $$list; do \
	  echo " $(INSTALL_DATA) '$$p' '$(DESTDIR)$(docdir)/$$p'"; \
	  $(INSTALL_DATA) "$$p" "$(DESTDIR)$(docdir)/$$p"; \
	done;

uninstall-local:
	@if test -n $(DESTDIR)$(docdir); then \
	  if test -d $(DESTDIR)$(docdir); then \
	    echo " rm -fr $(DESTDIR)$(docdir)/Wayland;"; \
	    rm -fr $(DESTDIR)$(docdir)/Wayland; \
	  fi; \
	fi;

endif
endif

EXTRA_DIST = \
	$(publican_sources) $(processed_sources) $(css_sources) $(img_sources) \
	protocol-to-docbook.xsl \
	protocol-interfaces-to-docbook.xsl \
	doxygen-to-publican.xsl \
	merge-mapcoords.xsl
