<?php
    header("Content-Security-Policy-Report-Only: script-src 'nonce-abc'");
?>
<!doctype html>
<script nonce="abc" src="/resources/testharness.js"></script>
<script nonce="abc" src="/resources/testharnessreport.js"></script>
<script nonce="abc">
    async_test(t => {
        var link = document.createElement('link');
        link.setAttribute("rel", "import");
        link.setAttribute("nonce", "zyx");
        link.setAttribute("href", "/security/resources/blank.html");
        link.onerror = t.unreached_func("The import should load.");
        link.onload = t.step_func(_ => assert_true(!!link.import, "The import was loaded."));

        var watcher = new EventWatcher(t, document, ['securitypolicyviolation', 'securitypolicyviolation']);
        watcher
            .wait_for('securitypolicyviolation')
            .then(t.step_func_done(e => {
                assert_equals(e.blockedURI, "http://127.0.0.1:8000/security/resources/blank.html");
                assert_equals(e.lineNumber, 21);
            }));

        document.head.appendChild(link);
    }, "Nonced imports load, and do not trigger reports.");
</script>
