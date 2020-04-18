#!/usr/bin/perl

use IO::Socket;

$| = 1;

autoflush STDOUT 1;

print "Content-Type: text/html; charset=us-ascii\n";
print "Transfer-encoding: chunked\n\n";

sleep 2;

print "hoge\nfuga\n";
