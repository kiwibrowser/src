#!/usr/bin/perl -wT
use strict;

print "X-Content-Type-Options: nosniff\n";
print "Content-Type: text/plain\n\n";

print "document.sniffing = true;\n";
