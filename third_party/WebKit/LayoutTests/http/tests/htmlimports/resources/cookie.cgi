#!/usr/bin/perl -wT
use strict;

print "Content-Type: text/html\n";
print "Cache-Control: no-store\n";
print 'Cache-Control: no-cache="set-cookie"' . "\n";
print "Set-Cookie: htmlimport=hello\n\n";

print "<html><body><h1>Cookie Sent!</h1></body></html>\n";