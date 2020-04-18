#!/usr/bin/perl

use IO::Socket;

$| = 1;

autoflush STDOUT 1;

print "Content-Type: text/html\n\n";

print "<html><head><script>alert(document.inputEncoding);</script>\n";
sleep 1;
print "<meta charset=windows-1255></head><body><script>alert(document.inputEncoding);</script></body></html>\n";
