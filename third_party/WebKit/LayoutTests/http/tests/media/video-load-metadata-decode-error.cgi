#!/usr/bin/perl -wT

use strict;

use CGI;
use File::stat;

my $query = new CGI;

my $name = $query->param('name');
my $filesize = stat($name)->size;

# Get MIME type.
my $type = $query->param('type');
if (!$type) {
    print "Status: 400 Bad Request\r\n";
    return;
}

my $rangeEnd = $filesize - 1;

# Print HTTP Header, disabling cache.
print "Cache-Control: no-cache\n";
print "Content-Length: " . $filesize . "\n";
print "Content-Type: " . $type . "\n";

print "\n";

open FILE, $name or die;
binmode FILE;
my ($data, $n);
my $total = 0;
my $break = $filesize  * 3 / 4;
my $string = "corrupt video";
seek(FILE, 0, 0);

while (($n = read FILE, $data, 1024) != 0) {
    print $data;

    $total += $n;
    if ($total >= $filesize) {
        last;
    }
    
    if ($total >= $break) {
        print $string;
        $total += length($string);
    }

}
close(FILE);
