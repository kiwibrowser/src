#!/usr/bin/perl
# http://trac.webkit.org/projects/webkit/changeset/7800
# rdar://problem/3829452 REGRESSION (156-157): onload handler doesn't run on page with meta refresh of 0 duration (new Apple start page)

# flush the buffers after each print
select (STDOUT);
$| = 1;

print "Refresh: 0;url=data:text/html,<body onload='if (window.testRunner) testRunner.notifyDone();'>You should have seen an alert.\r\n";
print "Content-Type: text/html\r\n";
print "\r\n";

print << "EOF";
<html>
<head>
<script>
if (window.testRunner) {
    testRunner.dumpAsText();
    testRunner.waitUntilDone();
}
</script>
<meta http-equiv="refresh" content="0;url=resources/notify-success-on-load.html">
</head>
EOF

for ($count=1; $count<20000; $count++) {
    print " \n";
}

print << "EOF";
<body onload="alert('SUCCESS');">
</body>
</html>
EOF
