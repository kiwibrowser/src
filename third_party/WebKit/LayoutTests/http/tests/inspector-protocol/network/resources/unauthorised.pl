#!/usr/bin/perl
use MIME::Base64;

if (!defined $ENV{HTTP_AUTHORIZATION}) {
  print "Status: 401 Unauthorized\r\n";
  print "WWW-Authenticate: Basic realm=\"WebKit Test Realm\"\r\n\r\n";
} else {
  my $auth = decode_base64(substr($ENV{HTTP_AUTHORIZATION},6));

  if ($auth eq "TestUser:TestPassword") {
    print "Content-type: text/javascript\r\n\r\n";
    print "console.log('Credentials accepted!');";
  } else {
    print "Status: 401 Unauthorized\r\n";
    print "WWW-Authenticate: Basic realm=\"WebKit Test Realm\"\r\n\r\n";
  }
}
