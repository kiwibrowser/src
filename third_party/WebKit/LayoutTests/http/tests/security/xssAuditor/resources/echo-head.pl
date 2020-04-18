#!/usr/bin/perl -wT
use strict;
use CGI;

my $cgi = new CGI;

print "X-XSS-Protection: 1\n";
print "Content-Type: text/html; charset=UTF-8\n\n";

print "<!DOCTYPE html>\n";
print "<html>\n";
print "<head>\n";
print $cgi->param('q');
print "</head>\n";
print "<body>\n";
print "</body>\n";
print "</html>\n";
