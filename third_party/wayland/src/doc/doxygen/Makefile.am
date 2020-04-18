
.SUFFIXES = .gv .png .map

noinst_DATA = \
              xml/Client/index.xml \
              xml/Server/index.xml \
              html/Client/index.html \
              html/Server/index.html
dist_noinst_DATA = wayland.doxygen.in

scanned_src_files_shared = 				\
	$(top_srcdir)/src/wayland-util.h

scanned_src_files_Client = 				\
	$(scanned_src_files_shared)			\
	$(top_srcdir)/src/wayland-client.c	\
	$(top_srcdir)/src/wayland-client.h	\
	$(top_srcdir)/src/wayland-client-core.h

scanned_src_files_Server = 				\
	$(scanned_src_files_shared)			\
	$(top_srcdir)/src/wayland-server.c	\
	$(top_srcdir)/src/wayland-server.h	\
	$(top_srcdir)/src/wayland-server-core.h	\
	$(top_srcdir)/src/wayland-shm.c

scanned_src_files_man =					\
	$(scanned_src_files_Server)			\
	$(top_srcdir)/src/wayland-client.c	\
	$(top_srcdir)/src/wayland-client.h	\
	$(top_srcdir)/src/wayland-client-core.h

extra_doxygen = \
	mainpage.dox

extra_doxygen_Server = \
	$(top_builddir)/protocol/wayland-server-protocol.h \
	$(extra_doxygen)

extra_doxygen_Client = \
	$(top_builddir)/protocol/wayland-client-protocol.h \
	$(extra_doxygen)

diagramsdir := $(srcdir)/dot
diagramssrc := $(wildcard $(diagramsdir)/*.gv)
diagrams := $(patsubst $(diagramsdir)/%,xml/%,$(diagramssrc:.gv=.png))
diagram_maps := $(patsubst $(diagramsdir)/%,xml/%,$(diagramssrc:.gv=.map))

# find all man/man3/wl_foo.3 pages
# for this to work, we need to create them before the man target (hence
# all-local below)
dist_man3_MANS = $(shell test -d man && find man/man3 -name "wl_*.3" -printf "man/man3/%P\n")

# Listing various directories that might need to be created.
alldirs := xml xml/Client xml/Server man/man3 html/Client html/Server

$(diagrams): $(diagramssrc)

$(diagram_maps):  $(diagramssrc)

xml/%/index.xml: $(top_srcdir)/src/scanner.c $(scanned_src_files_%) wayland.doxygen $(diagrams) $(diagram_maps) | xml/%
	$(AM_V_GEN)(cat wayland.doxygen; \
          echo "GENERATE_XML=YES"; \
          echo "XML_OUTPUT=xml/$*"; \
          echo "INPUT= $(scanned_src_files_$*)"; \
          ) | $(DOXYGEN) -

html/%/index.html: $(scanned_src_files_%) wayland.doxygen $(diagrams) $(diagram_maps) | html/%
	$(AM_V_GEN)(cat wayland.doxygen; \
          echo "PROJECT_NAME=\"Wayland $* API\""; \
          echo "GENERATE_HTML=YES"; \
          echo "HTML_OUTPUT=html/$*"; \
          echo "INPUT= $(scanned_src_files_$*) $(extra_doxygen_$*)"; \
          ) | $(DOXYGEN) -

man/man3/wl_display.3: $(top_srcdir)/src/scanner.c $(scanned_src_files_man) wayland.doxygen | man/man3
	$(AM_V_GEN)(cat wayland.doxygen; \
          echo "GENERATE_MAN=YES"; \
          echo "MAN_OUTPUT=man"; \
          echo "JAVADOC_AUTOBRIEF=NO"; \
          echo "INPUT= $(scanned_src_files_man)"; \
          ) | $(DOXYGEN) -

xml/%.png: $(diagramsdir)/%.gv | xml
	$(AM_V_GEN)$(DOT) -Tpng -o$@ $<

xml/%.map: $(diagramsdir)/%.gv | xml
	$(AM_V_GEN)$(DOT) -Tcmapx_np -o$@ $<

# general rule to create one of the listed directories.
$(alldirs):
	$(AM_V_GEN)$(MKDIR_P) $@

# there is no man-local
all-local: man/man3/wl_display.3

clean-local:
	rm -rf xml/
	rm -rf html/
	rm -rf man/

EXTRA_DIST = $(diagramssrc)
