#!/usr/bin/perl -wT

# flush the buffers after each print
select (STDOUT);
$| = 1;

print "Content-Type: application/xml\n";
print "Expires: Thu, 01 Dec 2003 16:00:00 GMT\n";
print "Cache-Control: no-store, no-cache, must-revalidate\n";
print "Pragma: no-cache\n";
print "\n";

print "\xef\xbb\xbf<?xml version=\"1.0\"?>\n";
print "<?xml-stylesheet type=\"text/xsl\" href=\"resources/transform.xsl\"?>\n";

for ($count=1; $count<4000; $count++) {
  print "\n";
}

print "<RESULT>FAILED</RESULT>\n";
