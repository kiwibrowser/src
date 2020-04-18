#!/usr/bin/perl -wT
use strict;
use Time::HiRes qw(sleep);

print "Content-Type: text/html\n\n";
sleep 1.0;
print <<EOF
<x-hello id="hello-slow"></x-hello>
<x-bye id="bye-slow"></x-bye>
EOF
