#!/usr/bin/perl -wT
use strict;

# CGI::Util::escape without any unicode cleverness.
sub escape {
    my $value = shift;
    $value =~ s/([^a-zA-Z0-9_.~-])/uc(sprintf("%%%02x", ord($1)))/eg;
    return $value;
}

print "Content-Type: text/plain\n";
print "Cache-Control: no-store\n";
print "Custom-Header: \xd5K\n\n";

# Unlike the latin-1 headers, the body is utf-8. To avoid this getting in the
# way of our tests, just escape everything. Character sets are the best.

foreach (keys %ENV) {
    if ($_ =~ "HTTP_") {
        print $_ . ": " . escape($ENV{$_}) . "\n";
    }
}
