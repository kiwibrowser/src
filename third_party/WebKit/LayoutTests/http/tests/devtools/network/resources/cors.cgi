#!/usr/bin/perl -wT
use strict;

print "Status: 200 OK\n";
print "Access-Control-Allow-Headers:X-PINGARUNER, CONTENT-TYPE\n";
print "Access-Control-Allow-Methods:POST, GET, OPTIONS\n";
print "Access-Control-Allow-Origin:http://127.0.0.1:8000\n";
print "Access-Control-Max-Age:1728000\n";
print "\n";
print "OK";
