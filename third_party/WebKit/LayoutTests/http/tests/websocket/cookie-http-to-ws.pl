#!/usr/bin/perl -wT
use strict;

my $originPath = $ENV{"SCRIPT_NAME"};

if ($ENV{"QUERY_STRING"} eq "clear=1") {
    print "Content-Type: text/plain\r\n",
          "Set-Cookie: ws-path-origin-script=0; Path=$originPath; Max-Age=0\r\n",
          "Set-Cookie: ws-path-root-domain-local-ip=0; Path=/; Domain=127.0.0.1; Max-Age=0\r\n",
          "\r\n",
          "Cookies are cleared.";
    exit;
}

print "Content-Type: text/html\r\n",
# Test that even if the "Path" attribute of a cookie matches the path of the
# origin document, the cookie won't be sent in the WebSocket handshake unless
# the "Path" attribute matches the WebSocket URL.
      "Set-Cookie: ws-path-origin-script=1; Path=$originPath\r\n",
# Test that if the "Path" and "Domain" matches the WebSocket URL, the cookie
# will be sent in the WebSocket handshake. "Path" is set to / so that the
# WebSocket created below can pass "Path" check so that we can test that
# "Domain" checking is working.
      "Set-Cookie: ws-path-root-domain-local-ip=1; Path=/; Domain=127.0.0.1\r\n",
      "\r\n";
print <<'HTML';
<script src="/js-test-resources/js-test.js"></script>
<script src="resources/get-request-header.js"></script>
<script>
description('Test how WebSocket handles cookies with cookie attributes.');

window.jsTestIsAsync = true;

// Normalize a cookie string
function normalizeCookie(cookie)
{
    // Split the cookie string, sort it and then put it back together.
    return cookie.split('; ').sort().join('; ');
}

function clearCookies()
{
    return new Promise(function(resolve, reject)
    {
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "cookie-http-to-ws.pl?clear=1");
        xhr.onreadystatechange = function()
        {
            if (xhr.readyState == 4) {
                resolve();
            }
        };
        xhr.send(null);
    });
}

var cookie = normalizeCookie(document.cookie);

shouldBeEqualToString('cookie', 'ws-path-origin-script=1; ws-path-root-domain-local-ip=1');

connectAndGetRequestHeader('cookie').then(function(value)
{
    cookie = value;
    shouldBeEqualToString('cookie', 'ws-path-root-domain-local-ip=1');
    clearCookies().then(finishJSTest);
}, finishAsFailed);
</script>
HTML
