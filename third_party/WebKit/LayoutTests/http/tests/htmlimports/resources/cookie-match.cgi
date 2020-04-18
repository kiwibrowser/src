#!/usr/bin/perl -wT
use strict;

my $query = $ENV{"QUERY_STRING"};
my $cookie = $ENV{"HTTP_COOKIE"};

$query =~ s/\?//g; # Squash "?" that is added to prevent dedup.

print "Content-Type: text/html\n";
print "Access-Control-Allow-Credentials: true\n";
print "Access-Control-Allow-Origin: *\n";
print "\n";

if ($query eq $cookie) {
    print "<body>PASS</body>"
} else {
    print "<body>FAIL: Cookie:" . $cookie .", Query:" . $query . "</body>";
}
