#! /bin/sh

#  Copyright (C) 2009, 2010 Christian Egli

# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.

# Use this script to regenerate Makefile.am if you must. I recommend
# against it for the reasons explained in
# http://www.gnu.org/software/hello/manual/automake/Wildcards.html.
# It's easy to pick up some spurious files that you did not mean to
# distribute.
 
OUTFILE=Makefile.am.new

(
cat <<'EOF' 
# generate the list of tables as follows:
# $ ls | grep -v Makefile | grep -v README | grep -v maketablelist.sh | grep -v '.*~$' | sort | sed -e 's/$/ \\/' -e 's/^/\t/' | head --bytes=-2
table_files = \
EOF
) > $OUTFILE

ls | grep -v Makefile | grep -v README | grep -v maketablelist.sh | grep -v '.*~$' | sort | sed -e 's/$/ \\/' -e 's/^/\t/' | head --bytes=-2 >> $OUTFILE

(
cat <<'EOF' 


tablesdir = $(datadir)/liblouis/tables
tables_DATA = $(table_files)
EXTRA_DIST = $(table_files)
EOF
) >> $OUTFILE

