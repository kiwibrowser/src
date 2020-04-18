<?php
    header("Content-Security-Policy-Report-Only: require-sri-for style; script-src 'self' 'unsafe-inline'");
?>
<!doctype html>
<script src="/resources/testharness.js"></script>
<script src="/resources/testharnessreport.js"></script>
<link rel="stylesheet" crossorigin integrity="sha256-48sSy1L+0pGBMr3XQog56zBcXid1hhmpAwenUuKoe5w=" href="/security/contentSecurityPolicy/resources/style-set-red.css">
<script>
    async_test(t => {
        var watcher = new EventWatcher(t, document, ['securitypolicyviolation']);
        watcher
            .wait_for('securitypolicyviolation')
            .then(t.step_func_done(e => {
                assert_equals(e.blockedURI, "http://127.0.0.1:8000/security/contentSecurityPolicy/blue.css");
                assert_equals(e.lineNumber, 16);
            }));
    }, "Stylesheets without integrity generate reports.");
</script>
<link rel="stylesheet" href="/security/contentSecurityPolicy/blue.css">
<script>
    async_test(t => {
        window.onload = t.step_func_done(_ => {
            assert_equals(document.styleSheets.length, 2);
            assert_equals(document.styleSheets[0].href, "http://127.0.0.1:8000/security/contentSecurityPolicy/resources/style-set-red.css");
            assert_equals(document.styleSheets[1].href, "http://127.0.0.1:8000/security/contentSecurityPolicy/blue.css");
        });
    }, "Stylesheet with integrity loads, and does not trigger reports.");
</script>