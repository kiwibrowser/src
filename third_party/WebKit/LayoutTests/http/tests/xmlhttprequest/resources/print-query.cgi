#!/usr/bin/perl -wT

use CGI qw(:standard);
my $cgi = new CGI;

print "Content-type: text/plain\n\n"; 
print $ENV{"QUERY_STRING"};
