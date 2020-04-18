#!/usr/bin/perl -wT
use strict;

print "Content-Type: text/html\n";
print "Access-Control-Allow-Credentials: true\n";
print "Access-Control-Allow-Origin: http://127.0.0.1:8000\n\n";

print <<EOF
<!DOCTYPE html>
<html>
<head>
<script src="http://127.0.0.1:8000/htmlimports/resources/external-script.js"></script>
<script src="http://localhost:8000/htmlimports/resources/cors-js.cgi"></script>
<script>
document.inlineScriptHasRun = true;
eval("document.evalFromInlineHasRun = true;");
</script>

</head>
</html>
EOF
