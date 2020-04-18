#!/usr/bin/perl

use IO::Socket;

$| = 1;

autoflush STDOUT 1;

print "Content-Type: text/html\n\n";

print "<html><head><title>\x59\xfd\x6c\x6d\x61\x7a\x20\xd6\x5a\x44\xdd\x4c</title>";
print "<script>function print(s) { var codes = ''; for (var i = 0; i < s.length; ++i) { codes = codes + '0x' + s.charCodeAt(i).toString(16) + ' '; } alert(codes); }</script>";
print "<script>print(document.title);</script>\n";
sleep 1;
print "<meta charset=windows-1254></head><body><script>print(document.title);</script></body></html>\n";
