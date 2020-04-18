doc_DATA = \
	liblouis.html \
	liblouis.txt

EXTRA_DIST = \
	liblouis.html \
	liblouis.txt

CLEANFILES = $(EXTRA_DIST)

info_TEXINFOS = liblouis.texi

# generate one big html file
AM_MAKEINFOHTMLFLAGS = --no-headers --no-split

SUFFIXES                = .txt

.texi.txt:
	$(MAKEINFO) --plaintext $< -o $@
