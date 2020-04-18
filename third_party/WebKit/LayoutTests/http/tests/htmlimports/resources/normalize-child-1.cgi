#!/usr/bin/perl
use strict;
use Time::HiRes qw(sleep);

print "Content-Type: text/html\n\n";
sleep 0.5;
print <<EOF
<link rel="import" href="normalize-child-2.html">
<script>
recordImported();
</script>
EOF
