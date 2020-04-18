#!/usr/bin/perl -wT
use strict;

print "Content-Type: text/javascript\n";
print "Access-Control-Allow-Credentials: true\n";
print "Access-Control-Allow-Origin: http://127.0.0.1:8000\n\n";

print "document.corsExternalScriptHasRun = true;\n";
print "eval(\"document.evalFromCorsExternalHasRun = true;\");";
