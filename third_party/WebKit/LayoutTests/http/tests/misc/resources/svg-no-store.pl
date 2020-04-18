#!/usr/bin/perl -wT

print "Content-Type: image/svg+xml\r\n";
print "Expires: Thu, 01 Dec 2003 16:00:00 GMT\r\n";
print "Cache-Control: no-store, no-cache, must-revalidate\r\n";
print "Pragma: no-cache\r\n";
print "\r\n";

open(FILE, "embedded.svg");
while (<FILE>) {
  print $_;
}
close(FILE);
