#!/usr/bin/perl -wT

use Time::HiRes;

# flush the buffers after each print
select (STDOUT);
$| = 1;

print "Content-Type: text/html\n\n";

sub sendChunk() {
    print "&#x0020;&#x0020; &#x0020;&#x0020;&#x0020;&#x0020;&#x20;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;  ";
    print "&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020; ";
    print "&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020; &#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020; ";
    print "&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020; ";
    print "&#x0020;&#x0020;&#x0020;&#x0020;&#x020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020; ";
    print "&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x020;&#x0020;&#x0020;&#x0020;&#x020;&#x0020;&#x0020;&#x0020;";
    print "&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x20;&#x0020;&#x0020;&#x0020; &#x0020;&#x0020;&#x0020;&#x0020;&#x0020; ";
    print "&#x0020;&#x0020;&#x0020; &#x20; &#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020;&#x0020; ";
}

sendChunk();
# Delay for 1 s which should be long enough to exit the 50 ms slot of XHR's
# ProgressEvent throttle (very long considering slow builds).
Time::HiRes::sleep(1);
sendChunk();
