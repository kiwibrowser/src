#!/usr/bin/perl
use warnings;
use strict;
$|++;

# Test a specific table with lou_checktable which causes an endless loop.
#
# Copyright (C) 2010 by Swiss Library for the Blind, Visually Impaired and Print Disabled
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.

my $table="loop.ctb";

$SIG{ALRM} = sub { die "lou_checktable on $table stuck in endless loop\n" };

alarm 5;
system("../tools/lou_checktable $table --quiet") == 0 
    or die "lou_checktable on $table failed\n";
alarm 0;
