#!/usr/bin/perl -wT
use strict;
use CGI;

my $cgi = new CGI;

print "X-XSS-Protection: 1\n";
print "Content-Type: text/html; charset=UTF-8\n\n";

print "<!DOCTYPE html>\n";
print "<html>\n";
print "<script>\n";
print "onload = function() {\n";
print "    window.parent.postMessage('loaded', '*');\n";
print "}\n";
print "</script>\n";
print "<body>\n";
print "<object id=\"object\" name=\"plugin\" type=\"application/x-blink-test-plugin\">\n";
print "<param name=\"movie\" value=\"".$cgi->param('q')."\" />\n";
print "</object>\n";
print "</body>\n";
print "</html>\n";
