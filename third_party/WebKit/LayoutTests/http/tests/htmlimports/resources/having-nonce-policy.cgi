#!/usr/bin/perl -wT
use strict;

print "Content-Type: text/html\n";
print "Access-Control-Allow-Credentials: true\n";
print "Access-Control-Allow-Origin: http://127.0.0.1:8000\n\n";

print <<EOF
<!DOCTYPE html>
<html>
<head>
<script src="http://localhost:8080/htmlimports/resources/external-script.js"></script>
<script nonce="hello" src="http://localhost:8080/htmlimports/resources/external-script-with-nonce.js"></script>
<script>
document.inlineScriptHasRun = true;
eval("document.evalFromInlineHasRun = true;");
</script>

<script nonce="hello">
document.inlineScriptWithNonceHasRun = true;
eval("document.evalFromInlineWithNonceHasRun = true;");
</script>
</head>
</html>
EOF
