# Makefile for Sphinx documentation
#

# You can set these variables from the command line.
SPHINXOPTS    = -W
SPHINXBUILD   = sphinx-build
BUILDDIR      = _build
CHROMESITE_BUILDDIR = $(realpath ../../doc_generated)
PORT = 8000

# User-friendly check for sphinx-build
ifeq ($(shell which $(SPHINXBUILD) >/dev/null 2>&1; echo $$?), 1)
$(error The '$(SPHINXBUILD)' command was not found. Make sure you have Sphinx installed, then set the SPHINXBUILD environment variable to point to the full path of the '$(SPHINXBUILD)' executable. Alternatively you can add the directory with the executable to your PATH. If you don't have Sphinx installed, grab it from http://sphinx-doc.org/)
endif

# Internal variables.
ALLSPHINXOPTS   = -d $(BUILDDIR)/doctrees $(SPHINXOPTS) .

.PHONY: all help clean linkcheck doctest chromesite chromesite_rst serve

all: chromesite

help:
	@echo "Please use \`make <target>' where <target> is one of"
	@echo "  html            to make standalone HTML files"
	@echo "  chromesite      build docs for developer.chrome.com"
	@echo "  chromesite_rst  only build .rst docs; no doxygen"
	@echo "  serve           start python web server for chromesite docs"
	@echo "  linkcheck       to check all external links for integrity"
	@echo "  doctest         to run all doctests embedded in the documentation (if enabled)"
	@echo "  presubmit       build docs in a temp directory (for presubmit check)"

clean:
	rm -rf $(BUILDDIR)/*

html:
	$(SPHINXBUILD) -b html $(ALLSPHINXOPTS) $(BUILDDIR)/html
	@echo
	@echo "Build finished. The HTML pages are in $(BUILDDIR)/html."

linkcheck:
	$(SPHINXBUILD) -b linkcheck $(ALLSPHINXOPTS) $(BUILDDIR)/linkcheck
	@echo
	@echo "Link check complete; look for any errors in the above output " \
	      "or in $(BUILDDIR)/linkcheck/output.txt."

doctest:
	$(SPHINXBUILD) -b doctest $(ALLSPHINXOPTS) $(BUILDDIR)/doctest
	@echo "Testing of doctests in the sources finished, look at the " \
	      "results in $(BUILDDIR)/doctest/output.txt."

chromesite:
	-rm -f $(CHROMESITE_BUILDDIR)/.buildinfo
	doxygen/generate_docs.py $(CHROMESITE_BUILDDIR)
	$(SPHINXBUILD) -b chromesite $(ALLSPHINXOPTS) $(CHROMESITE_BUILDDIR)
	rm -rf $(CHROMESITE_BUILDDIR)/images
	cp -r $(CHROMESITE_BUILDDIR)/_images $(CHROMESITE_BUILDDIR)/images
	rm -rf $(CHROMESITE_BUILDDIR)/{_images,searchindex.js}
	@echo
	@echo "Build finished. The HTML pages are in $(CHROMESITE_BUILDDIR)"

chromesite_rst:
	-rm $(CHROMESITE_BUILDDIR)/.buildinfo
	$(SPHINXBUILD) -b chromesite $(ALLSPHINXOPTS) $(CHROMESITE_BUILDDIR)
	rm -rf $(CHROMESITE_BUILDDIR)/images
	cp -r $(CHROMESITE_BUILDDIR)/_images $(CHROMESITE_BUILDDIR)/images
	rm -rf $(CHROMESITE_BUILDDIR)/{_images,searchindex.js}

presubmit:
	-rm $(BUILDDIR)/.buildinfo
	$(SPHINXBUILD) -b chromesite $(ALLSPHINXOPTS) $(BUILDDIR)

serve: chromesite_rst
	../../../chrome/common/extensions/docs/server2/preview.py -p $(PORT)
