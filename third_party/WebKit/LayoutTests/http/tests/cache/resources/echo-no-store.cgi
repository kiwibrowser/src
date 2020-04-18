#!/usr/bin/perl -wT

print "Content-type: text/plain\n"; 
print "Cache-control: no-store\n";
print "\n";
read(STDIN, $data, $ENV{'CONTENT_LENGTH'});
print $data
