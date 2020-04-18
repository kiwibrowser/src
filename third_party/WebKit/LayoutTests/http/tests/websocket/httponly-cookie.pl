#!/usr/bin/perl -wT
use strict;

if ($ENV{"QUERY_STRING"} eq "clear=1") {
    print "Content-Type: text/plain\r\n",
          "Set-Cookie: ws=0; Path=/; Max-Age=0\r\n",
          "Set-Cookie: ws-httponly=0; Path=/; HttpOnly; Max-Age=0\r\n",
          "\r\n",
          "Cookies are cleared.";
    exit;
}

print "Content-Type: text/html\r\n",
# The "Path" attribute is set to "/" so that the WebSocket created below
# will receive these cookies.
      "Set-Cookie: ws=1; Path=/\r\n",
      "Set-Cookie: ws-httponly=1; Path=/; HttpOnly\r\n",
      "\r\n";
print <<'HTML';
<!DOCTYPE html>
<script src="/js-test-resources/js-test.js"></script>
<script src="resources/get-request-header.js"></script>
<script>
description('Test that WebSocket sends HttpOnly cookies.');

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
        xhr.open("GET", "httponly-cookie.pl?clear=1");
        xhr.onreadystatechange = function()
        {
            if (xhr.readyState == 4) {
                resolve();
            }
        };
        xhr.send(null);
    });
}

var cookie;
connectAndGetRequestHeader('cookie').then(function(value)
{
    cookie = value;
    cookie = normalizeCookie(cookie);
    shouldBeEqualToString('cookie', 'ws-httponly=1; ws=1');
    clearCookies().then(finishJSTest);
}, finishAsFailed);

setTimeout(finishJSTest, 1000);

</script>
HTML
