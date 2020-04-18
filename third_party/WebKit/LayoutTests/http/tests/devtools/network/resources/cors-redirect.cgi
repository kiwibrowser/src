#!/usr/bin/perl -wT
use strict;
use CGI;
my $q = CGI->new;
my %headers = map { $_ => $q->http($_) } $q->http();

print "Status: 307 Internal Redirect\n";
print "Location: cors.cgi\n";
print "Access-Control-Allow-Headers:X-PINGARUNER, CONTENT-TYPE, X-TEST\n";
print "Access-Control-Allow-Methods:POST, GET, OPTIONS\n";
print "Access-Control-Allow-Origin:http://127.0.0.1:8000\n";
print "Access-Control-Max-Age:1728000\n";
print "\n";
print "OK";
