<?php
    header("Content-Security-Policy-Report-Only: style-src 'nonce-abc'");
?>
<!doctype html>
<script src="/resources/testharness.js"></script>
<script src="/resources/testharnessreport.js"></script>
<script>
    async_test(t => {
        var watcher = new EventWatcher(t, document, ['securitypolicyviolation','securitypolicyviolation']);
        watcher
            .wait_for('securitypolicyviolation')
            .then(t.step_func(e => {
                assert_equals(e.blockedURI, "inline");
                assert_equals(e.lineNumber, 20);
                return watcher.wait_for('securitypolicyviolation');
            }))
            .then(t.step_func_done(e => {
                assert_equals(e.blockedURI, "http://127.0.0.1:8000/security/contentSecurityPolicy/style-set-red.css");
                assert_equals(e.lineNumber, 25);
            }));
    }, "Incorrectly nonced style blocks generate reports.");
</script>
<style>
    #test1 {
        color: rgba(1,1,1,1);
    }
</style>
<link rel="stylesheet" href="/security/contentSecurityPolicy/style-set-red.css" nonce="xyz">
<script>
    async_test(t => {
        window.onload = t.step_func_done(_ => {
            assert_equals(document.styleSheets.length, 2);
            assert_equals(document.styleSheets[0].href, null);
            assert_equals(document.styleSheets[1].href, "http://127.0.0.1:8000/security/contentSecurityPolicy/style-set-red.css");
        });
    }, "Incorrectly nonced stylesheets load.");
</script>
