#!/usr/bin/perl -wT
use strict;

print "Content-Type: text/html\n";
print "Access-Control-Allow-Credentials: true\n";
print "Access-Control-Allow-Origin: http://127.0.0.1:8000\n\n";

print <<EOF
<!DOCTYPE html>
<html>
<head>
<link rel="import" href="having-nonce-policy.cgi">
</head>
</html>
EOF
