<?php
    header("Content-Security-Policy: require-sri-for style; script-src 'self' 'unsafe-inline'");
?>
<!doctype html>
<script src="/resources/testharness.js"></script>
<script src="/resources/testharnessreport.js"></script>
<link rel="stylesheet" crossorigin integrity="sha256-48sSy1L+0pGBMr3XQog56zBcXid1hhmpAwenUuKoe5w=" href="/security/contentSecurityPolicy/resources/style-set-red.css">
<script>
    async_test(t => {
        document.addEventListener('securitypolicyviolation', t.unreached_func("No report should be generated"));
        window.onload = t.step_func_done(_ => {
            assert_equals(document.styleSheets.length, 1);
            assert_equals(document.styleSheets[0].href, "http://127.0.0.1:8000/security/contentSecurityPolicy/resources/style-set-red.css");
        });
    }, "Stylesheet with integrity loads, and does not trigger reports.");
</script>