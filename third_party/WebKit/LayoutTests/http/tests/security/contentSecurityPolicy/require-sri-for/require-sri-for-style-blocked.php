<?php
    header("Content-Security-Policy: require-sri-for style;");
?>
<!doctype html>
<script src="/resources/testharness.js"></script>
<script src="/resources/testharnessreport.js"></script>
<script>
    async_test(t => {
        var watcher = new EventWatcher(t, document, ['securitypolicyviolation']);
        watcher
            .wait_for('securitypolicyviolation')
            .then(t.step_func_done(e => {
                assert_equals(e.blockedURI, "http://127.0.0.1:8000/security/contentSecurityPolicy/resources/style-set-red.css");
                assert_equals(e.lineNumber, 15);
            }));
    }, "Stylesheets without integrity generate reports.");
</script>
<link rel="stylesheet" href="/security/contentSecurityPolicy/resources/style-set-red.css">
<script>
    async_test(t => {
        window.onload = t.step_func_done(_ => {
            assert_equals(document.styleSheets.length, 1);
            assert_equals(document.styleSheets[0].rules.length, 0);
        });
    }, "Stylesheets without integrity do not load.");
</script>
