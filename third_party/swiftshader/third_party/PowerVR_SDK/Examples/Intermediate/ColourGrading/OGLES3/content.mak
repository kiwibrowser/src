#--------------------------------------------------------------------------
# Name         : content.mak
# Title        : Makefile to build content files
#
# Copyright    : Copyright (C) by Imagination Technologies Limited.
#
# Description  : Makefile to wrap content files for examples in the PowerVR SDK
#
# Platform     :
#
#--------------------------------------------------------------------------

#############################################################################
## Variables
#############################################################################
FILEWRAP 	= ..\..\..\..\Utilities\Filewrap\Windows_x86_32\Filewrap.exe
CONTENTDIR = Content

#############################################################################
## Instructions
#############################################################################

RESOURCES = \
	$(CONTENTDIR)/FragShader.cpp \
	$(CONTENTDIR)/VertShader.cpp \
	$(CONTENTDIR)/SceneFragShader.cpp \
	$(CONTENTDIR)/SceneVertShader.cpp \
	$(CONTENTDIR)/BackgroundFragShader.cpp \
	$(CONTENTDIR)/Mask.cpp \
	$(CONTENTDIR)/MaskTexture.cpp \
	$(CONTENTDIR)/Background.cpp \
	$(CONTENTDIR)/identity.cpp \
	$(CONTENTDIR)/cooler.cpp \
	$(CONTENTDIR)/warmer.cpp \
	$(CONTENTDIR)/bw.cpp \
	$(CONTENTDIR)/sepia.cpp \
	$(CONTENTDIR)/inverted.cpp \
	$(CONTENTDIR)/highcontrast.cpp \
	$(CONTENTDIR)/bluewhitegradient.cpp

all: resources
	
help:
	@echo Valid targets are:
	@echo resources, clean
	@echo FILEWRAP can be used to override the default path to the Filewrap utility.

clean:
	@for i in $(RESOURCES); do test -f $$i && rm -vf $$i || true; done

resources: $(RESOURCES)

$(CONTENTDIR):
	-mkdir "$@"

$(CONTENTDIR)/FragShader.cpp: $(CONTENTDIR) ./FragShader.fsh
	$(FILEWRAP)  -s  -o $@ ./FragShader.fsh

$(CONTENTDIR)/VertShader.cpp: $(CONTENTDIR) ./VertShader.vsh
	$(FILEWRAP)  -s  -o $@ ./VertShader.vsh

$(CONTENTDIR)/SceneFragShader.cpp: $(CONTENTDIR) ./SceneFragShader.fsh
	$(FILEWRAP)  -s  -o $@ ./SceneFragShader.fsh

$(CONTENTDIR)/SceneVertShader.cpp: $(CONTENTDIR) ./SceneVertShader.vsh
	$(FILEWRAP)  -s  -o $@ ./SceneVertShader.vsh

$(CONTENTDIR)/BackgroundFragShader.cpp: $(CONTENTDIR) ./BackgroundFragShader.fsh
	$(FILEWRAP)  -s  -o $@ ./BackgroundFragShader.fsh

$(CONTENTDIR)/Mask.cpp: $(CONTENTDIR) ./Mask.pod
	$(FILEWRAP)  -o $@ ./Mask.pod

$(CONTENTDIR)/MaskTexture.cpp: $(CONTENTDIR) ./MaskTexture.pvr
	$(FILEWRAP)  -o $@ ./MaskTexture.pvr

$(CONTENTDIR)/Background.cpp: $(CONTENTDIR) ./Background.pvr
	$(FILEWRAP)  -o $@ ./Background.pvr

$(CONTENTDIR)/identity.cpp: $(CONTENTDIR) ./identity.pvr
	$(FILEWRAP)  -o $@ ./identity.pvr

$(CONTENTDIR)/cooler.cpp: $(CONTENTDIR) ./cooler.pvr
	$(FILEWRAP)  -o $@ ./cooler.pvr

$(CONTENTDIR)/warmer.cpp: $(CONTENTDIR) ./warmer.pvr
	$(FILEWRAP)  -o $@ ./warmer.pvr

$(CONTENTDIR)/bw.cpp: $(CONTENTDIR) ./bw.pvr
	$(FILEWRAP)  -o $@ ./bw.pvr

$(CONTENTDIR)/sepia.cpp: $(CONTENTDIR) ./sepia.pvr
	$(FILEWRAP)  -o $@ ./sepia.pvr

$(CONTENTDIR)/inverted.cpp: $(CONTENTDIR) ./inverted.pvr
	$(FILEWRAP)  -o $@ ./inverted.pvr

$(CONTENTDIR)/highcontrast.cpp: $(CONTENTDIR) ./highcontrast.pvr
	$(FILEWRAP)  -o $@ ./highcontrast.pvr

$(CONTENTDIR)/bluewhitegradient.cpp: $(CONTENTDIR) ./bluewhitegradient.pvr
	$(FILEWRAP)  -o $@ ./bluewhitegradient.pvr

############################################################################
# End of file (content.mak)
############################################################################
