#!/usr/bin/perl -wT
use strict;

print "Content-Type: text/javascript\n";
print "Access-Control-Allow-Credentials: true\n";
print "Access-Control-Allow-Origin: http://localhost:8000\n\n";

print "document.corsExternalScriptForLocalhostHasRun = true;\n";
print "eval(\"document.evalFromCorsExternalLocalhostHasRun = true;\");";
