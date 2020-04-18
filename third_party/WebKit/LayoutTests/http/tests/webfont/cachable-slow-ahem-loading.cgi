#!/usr/bin/perl -wT
use CGI;
use HTTP::Date;
use Time::HiRes;

my $cgi = new CGI;
my $delay = $cgi->param('delay');
$delay = 1000 unless $delay;
my $now = time2str(time);

# Last-modified is needed so that memory cache decides to perform revalidation.
print "Last-modified: $now\n";
print "Content-type: application/octet-stream\n\n";
Time::HiRes::sleep($delay / 1000);
open FH, "<../resources/Ahem.ttf" or die;
while (<FH>) { print; }
close FH;
