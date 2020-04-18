#!/usr/bin/perl -wT
use strict;
use Time::HiRes qw(sleep);

sleep 0.2;

print "Content-Type: text/javascript\n\n";
print <<EOF
// Do nothing
EOF
