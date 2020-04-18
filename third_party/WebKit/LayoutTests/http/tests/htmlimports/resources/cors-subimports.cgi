#!/usr/bin/perl -wT
use strict;

print "Content-Type: text/html\n";
print "Access-Control-Allow-Credentials: true\n";
print "Access-Control-Allow-Origin: http://127.0.0.1:8000\n\n";

print <<EOF
<!DOCTYPE html>
<link id="sameOriginCors" rel="import" href="cors-basic.cgi?1">
<link id="sameOriginNoCors" rel="import" href="resources/hello.html?1">
<link id="masterOriginNoCors" rel="import" href="http://127.0.0.1:8000/htmlimports/resources/hello.html?2">
<link id="masterOriginCors" rel="import" href="http://127.0.0.1:8000/htmlimports/resources/cors-basic.cgi?2">
EOF

