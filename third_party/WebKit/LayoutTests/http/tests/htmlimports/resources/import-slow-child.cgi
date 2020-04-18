#!/usr/bin/perl -wT
use strict;
use Time::HiRes qw(sleep);

print "Content-Type: text/html\n\n";
sleep 0.2;
print <<EOF
<script>
window.childLoaded = true;
</script>
EOF
