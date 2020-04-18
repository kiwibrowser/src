#!/usr/bin/perl

# This script outputs a broken transfer-encoded response. In order to deliver
# didReceiveResponse to the client (i.e. XHR), we send a small valid chunk
# first, wait a while and then send an invalid chunk.

use IO::Socket;

$| = 1;

autoflush STDOUT 1;

print "Content-Type: text/html; charset=us-ascii\n";
print "X-Content-Type-Options: nosniff\n";
print "Transfer-encoding: chunked\n\n";

print "5\nhello\n";
sleep 2;

print "hoge\nfuga\n";
