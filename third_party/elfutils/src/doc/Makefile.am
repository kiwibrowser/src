## Process this file with automake to create Makefile.in
## Configure input file for elfutils.
##
## Copyright (C) 1996-2001, 2002, 2005 Red Hat, Inc.
## This file is part of elfutils.
##
## This file is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 3 of the License, or
## (at your option) any later version.
##
## elfutils is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.
EXTRA_DIST = elfutils.sgml

CLEANFILES = elfutils.dvi

# We need only a few special rules to generate the various output formats
# from the SGML sources.
.PHONY: dvi pdf html
pdf: $(srcdir)elfutils.pdf
dvi: $(srcdir)elfutils.dvi

$(srcdir)%.dvi: %.sgml
	db2dvi $^
$(srcdir)%.pdf: %.sgml
	db2pdf $^
