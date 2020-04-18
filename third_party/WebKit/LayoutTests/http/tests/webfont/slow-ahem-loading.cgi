#!/usr/bin/perl -wT
use CGI;
use Time::HiRes;

my $cgi = new CGI;
my $delay = $cgi->param('delay');
$delay = 1000 unless $delay;

print "Content-type: application/octet-stream\n";
print "Cache-control: no-cache, no-store\n\n";
Time::HiRes::sleep($delay / 1000);
open FH, "<../resources/Ahem.ttf" or die;
while (<FH>) { print; }
close FH;
