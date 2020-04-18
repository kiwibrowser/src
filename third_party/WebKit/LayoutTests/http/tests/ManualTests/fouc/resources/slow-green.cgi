#!/usr/bin/perl -wT
use strict;

print "Content-Type: text/html\n\n";
# If you want to make things super predictable, this will help for the cost of slowness.
# In practice, CGI invocation overhead is sufficiently slow.
sleep 1;
print <<EOF
<style>
  #test {
    background: green;
  }
</style>
EOF
