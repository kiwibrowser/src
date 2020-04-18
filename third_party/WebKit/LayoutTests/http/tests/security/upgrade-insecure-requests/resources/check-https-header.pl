#!/usr/bin/perl -wT
use strict;

print "Content-Type: text/html\n";
print "Access-Control-Allow-Origin: *\n";
print "Cache-Control: no-store\n\n";

print <<DONE
<!DOCTYPE html>
<html>
<head>
  <script src="/resources/testharness.js"></script>
  <script src="/resources/testharnessreport.js"></script>
  <script>
    test(function () {
      var httpsHeader = "$ENV{"HTTP_UPGRADE_INSECURE_REQUESTS"}";
      assert_equals(httpsHeader, "1");
    }, "Verify that this request was delivered with an 'Upgrade-Insecure-Requests' header.");
  </script>
</head>
</html>
DONE
