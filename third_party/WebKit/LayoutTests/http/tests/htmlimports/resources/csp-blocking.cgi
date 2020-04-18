#!/usr/bin/perl -wT
use strict;

print "Content-Type: text/html\n";
print "Content-Security-Policy: script-src 'unsafe-inline'\n\n";

print "<html><head>";
print "<link id=\"shouldBeBlocked\" rel=\"import\" href=\"hello.html\">";
print "</head></html>\n";
