#!/usr/bin/perl -wT
use strict;

print "Content-Type: text/html\n";
print "Access-Control-Allow-Credentials: true\n";
print "Access-Control-Allow-Origin: *\n";
print "\n";
print <<EOF
<!DOCTYPE html>
<!-- Put an extra "?" in the URL to prevent de-deup -->
<link id="sameOrigin" rel="import" href="http://127.0.0.1:8000/htmlimports/resources/cookie-match.cgi?key=HelloCredentials?">
EOF
