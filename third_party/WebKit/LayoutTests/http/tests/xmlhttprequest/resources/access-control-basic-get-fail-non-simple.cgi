#!/usr/bin/perl -wT
use strict;

my $method = $ENV{'REQUEST_METHOD'};

if ($method eq "OPTIONS") {
  # Reject preflights.
  print <<EOF
Status: 403 Forbidden
Content-Type: text/plain

EOF
} elsif ($method eq "GET") {
  # Respond to simple cross-origin requests.
  print <<EOF
Status: 200 OK
Content-Type: text/plain
Access-Control-Allow-Credentials: true
Access-Control-Allow-Origin: http://127.0.0.1:8000

Hello
EOF
} else {
  # Return a status code that is different from the OPTIONS case so that
  # layout tests can utilize it.
  print <<EOF
Status: 400 Bad Request
Content-Type: text/plain

EOF
}
