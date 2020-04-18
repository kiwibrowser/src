#!/usr/bin/perl -wT
use strict;
use warnings;
use CGI;

my $cgi = new CGI;
my $csp = $cgi->param('csp');
my $plugin = $cgi->param('plugin');
my $log = $cgi->param('log');
my $type = $cgi->param('type');

if ($type) {
  $type = qq/type="$type"/;
}

print qq,Content-Type: text/html; charset=UTF-8
Content-Security-Policy: $csp

<!DOCTYPE html>
<html>
<body>
<object data="$plugin" $type></object>
,;

if ($log) {
  print qq@<script>
function log(s) {
  var console = document.querySelector('#console');
  if (!console) {
    console = document.body.appendChild(document.createElement('div'));
    console.id = 'console';
  }
  console.appendChild(
      document.createElement('p')).appendChild(
          document.createTextNode(s));
}
if (document.querySelector('object').postMessage)
  log("$log");
</script>@;
}

print qq,
</body>
</html>
,;
