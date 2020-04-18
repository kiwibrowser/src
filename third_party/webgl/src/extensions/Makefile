# See README for more information about the WebGL extension registry automation.

EXTS := $(shell ./find-exts)
EXTS := $(strip $(EXTS))

EXT_XMLS := $(addsuffix /extension.xml,$(EXTS))
EXT_IDXS := $(addsuffix /index.html,$(EXTS))

# a list of resources to be built for validation
VALID := schematron/webgl_message.xsl extension-schematron.xsl

comma:=,
empty:=
space:=$(empty) $(empty)
EXT_LIST:=$(subst $(space),$(comma),$(EXTS))

# This sed substitute command builds a relative path back to the SVN root
# from an extension registry relative path
RE_BASEPATH:="s/[^\/]*/../g"

.PHONY: all clean

all: index.html index.atom $(EXT_IDXS) template/index.html

index.html: registry.xml registry.xsl registry.html rev_utils.xsl
	xsltproc --stringparam basepath `echo "" | sed -e $(RE_BASEPATH)` \
	         --html registry.xsl registry.html > index.html

index.atom: registry.xml revisions.atom atom.xsl
	xsltproc atom.xsl revisions.atom > index.atom

registry.xml: $(EXT_XMLS) summary.xsl summary.xml
	xsltproc --stringparam extensions $(EXT_LIST) \
	         summary.xsl summary.xml > registry.xml

%/index.html: %/extension.xml extension.xsl rev_utils.xsl standards.xsl $(VALID)
	./validate $*
	xsltproc --stringparam basepath `echo $*/ | sed -e $(RE_BASEPATH)` \
	         extension.xsl $*/extension.xml > $*/index.html

schematron/webgl_message.xsl: schematron/iso_schematron_message.xsl
	xsltproc schematron/webgl_message_patch.xsl \
	         schematron/iso_schematron_message.xsl \
	       > schematron/webgl_message.xsl

extension-schematron.xsl: extension.sch schematron/webgl_message.xsl
	xsltproc schematron/iso_dsdl_include.xsl extension.sch \
	| xsltproc schematron/iso_abstract_expand.xsl - \
	| xsltproc schematron/webgl_message.xsl - \
	> extension-schematron.xsl

clean:
	rm -f index.html index.atom registry.xml
	rm -f $(EXT_IDXS) template/index.html
	rm -f $(VALID)
