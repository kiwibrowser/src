<?php
    header("Content-Security-Policy: script-src 'nonce-abcdefg' 'strict-dynamic', script-src * 'unsafe-inline'");
?>
<!DOCTYPE html>
<html>
<head>
    <script src="/resources/testharness.js" nonce="abcdefg"></script>
    <script src="/resources/testharnessreport.js" nonce="abcdefg"></script>
    <script src="../resources/securitypolicyviolation-helper.js" nonce="abcdefg"></script>
</head>
<body>
    <!-- Need to individually wrap test cases in script blocks. Violation reports triggered by document.write() calls while the parser is waiting on blocking scipts are missing line numbers. See: https://crbug.com/649085. -->
    <script nonce="abcdefg">
        async_test(function (t) {
          var e = document.createElement('script');
          e.src = generateURL("append");
          e.onerror = t.unreached_func("Error should not be triggered.");
          assert_script_loads(t, generateURL("append"), 16);
          document.body.appendChild(e);
        }, "Script injected via 'appendChild' is allowed with 'strict-dynamic'.");
    </script>
    <script nonce="abcdefg">
        async_test(function (t) {
          var e = document.createElement('script');
          e.src = generateURL("append-async");
          e.async = true;
          e.onerror = t.unreached_func("Error should not be triggered.");
          assert_script_loads(t, generateURL("append-async"), 26);
          document.body.appendChild(e);
        }, "Async script injected via 'appendChild' is allowed with 'strict-dynamic'.");
    </script>
    <script nonce="abcdefg">
        async_test(function (t) {
          var e = document.createElement('script');
          e.src = generateURL("append-defer");
          e.defer = true;
          e.onerror = t.unreached_func("Error should not be triggered.");
          assert_script_loads(t, generateURL("append-defer"), 26);
          document.body.appendChild(e);
        }, "Deferred script injected via 'appendChild' is allowed with 'strict-dynamic'.");
    </script>
    <script nonce="abcdefg">
        async_test(function (t) {
          assert_script_failed(t, generateURL("write"), 42);
          document.write("<scr" + "ipt src='" + generateURL("write") + "'></scr" + "ipt>");
        }, "Script injected via 'document.write' is not allowed with 'strict-dynamic'.");
    </script>
    <script nonce="abcdefg">
        async_test(function (t) {
          assert_script_failed(t, generateURL("write-defer"), 48);
          document.write("<scr" + "ipt defer src='" + generateURL("write-defer") + "'></scr" + "ipt>");
        }, "Deferred script injected via 'document.write' is not allowed with 'strict-dynamic'.");
    </script>
    <script nonce="abcdefg">
        async_test(function (t) {
          assert_script_failed(t, generateURL("write-async"), 54);
          document.write("<scr" + "ipt async src='" + generateURL("write-async") + "'></scr" + "ipt>");
        }, "Async script injected via 'document.write' is not allowed with 'strict-dynamic'.");
    </script>
    <script nonce="abcdefg" defer>
        async_test(function (t) {
          var e = document.createElement('script');
          e.src = generateURL("defer-append");
          e.onerror = t.unreached_func("Error should not be triggered.");
          assert_script_loads(t, generateURL("defer-append"), 63);
          document.body.appendChild(e);
        }, "Script injected via deferred 'appendChild' is allowed with 'strict-dynamic'.");
    </script>
    <script nonce="abcdefg" defer>
        async_test(function (t) {
          var e = document.createElement('script');
          e.src = generateURL("defer-append-async");
          e.async = true;
          e.onerror = t.unreached_func("Error should not be triggered.");
          assert_script_loads(t, generateURL("defer-append-async"), 73);
          document.body.appendChild(e);
        }, "Async script injected via deferred 'appendChild' is allowed with 'strict-dynamic'.");
    </script>
    <script nonce="abcdefg" defer>
        async_test(function (t) {
          var e = document.createElement('script');
          e.src = generateURL("defer-append-defer");
          e.defer = true;
          e.onerror = t.unreached_func("Error should not be triggered.");
          assert_script_loads(t, generateURL("defer-append-defer"), 83);
          document.body.appendChild(e);
        }, "Deferred script injected via deferred 'appendChild' is allowed with 'strict-dynamic'.");
    </script>
    <script nonce="abcdefg" defer>
        async_test(function (t) {
          assert_script_failed(t, generateURL("defer-write"), 89);
          document.write("<scr" + "ipt src='" + generateURL("defer-write") + "'></scr" + "ipt>");
        }, "Script injected via deferred 'document.write' is not allowed with 'strict-dynamic'.");
    </script>
    <script nonce="abcdefg" defer>
        async_test(function (t) {
          assert_script_failed(t, generateURL("defer-write-defer"), 95);
          document.write("<scr" + "ipt defer src='" + generateURL("defer-write-defer") + "'></scr" + "ipt>");
        }, "Deferred script injected via deferred 'document.write' is not allowed with 'strict-dynamic'.");
    </script>
    <script nonce="abcdefg" defer>
        async_test(function (t) {
          assert_script_failed(t, generateURL("defer-write-async"), 101);
          document.write("<scr" + "ipt async src='" + generateURL("defer-write-async") + "'></scr" + "ipt>");
        }, "Async script injected via deferred 'document.write' is not allowed with 'strict-dynamic'.");
    </script>
</body>
</html>
