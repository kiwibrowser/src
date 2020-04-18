#!/usr/bin/perl
use warnings;
use strict;
$|++;

# Test all tables with lou_checktable.
#
# Copyright (C) 2010 by Swiss Library for the Blind, Visually Impaired and Print Disabled
# Copyright (C) 2012-2013 Mesar Hameed <mhameed @ src.gnome.org>
# 
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.

my $fail = 0;
# some tables are quite big and take some time to check, so keep the timeout reasonably long
my $timeout = 120; # seconds

# We assume that the productive tables, i.e. the ones that are shipped
# with liblouis (and need to be tested) are found in the first path in
# LOUIS_TABLEPATH. The subsequent entries are for test tables.
my $tablesdir = (split(',', $ENV{LOUIS_TABLEPATH}))[0];

# get all the tables from the tables directory
my @tables = glob("$tablesdir/*.{utb,ctb}");


foreach my $table (@tables) {
    if (my $pid = fork) {
	waitpid($pid, 0);
	if ($?) {
	    print STDERR "lou_checktable on $table failed or timed out\n";
	    $fail = 1;
	}
    } else {
	die "cannot fork: $!" unless defined($pid);
	alarm $timeout;
	exec ("../tools/lou_checktable $table --quiet");
	die "Exec of lou_checktable failed: $!";
    }
}

exit $fail;
