<?php
    header("Content-Security-Policy: script-src 'nonce-abc'");
?>
<!doctype html>
<script nonce="abc" src="/resources/testharness.js"></script>
<script nonce="abc" src="/resources/testharnessreport.js"></script>
<script nonce="abc">
    // HTML imports are render-blocking, therefore the test must not complete
    // before the import finishes (or fails) loading, otherwise, the test output
    // will be empty (or at least flaky).
    //
    // This is because testharness.js appends the results as <pre> elements to the
    // DOM, and then dumps the document as text. However, in a non-rendering-ready
    // document, all elements are effectively `display: none`, and as such would
    // not appear in the dump.
    async_test(t => {
        window.onload = t.step_func(_ => {
            var link = document.createElement('link');
            link.setAttribute("rel", "import");
            link.setAttribute("nonce", "zyx");
            link.setAttribute("href", "/security/resources/blank.html");
            link.onload = t.unreached_func();
            link.onerror = t.step_func_done();
            document.body.appendChild(link);
        });
    }, "Incorrectly nonced imports do not load.");

    async_test(t => {
        var watcher = new EventWatcher(t, document, ['securitypolicyviolation']);
        watcher
            .wait_for('securitypolicyviolation')
            .then(t.step_func_done(e => {
                assert_equals(e.blockedURI, "http://127.0.0.1:8000/security/resources/blank.html");
                assert_equals(e.lineNumber, 21);
             }));
    }, "Incorrectly nonced imports trigger reports.");
</script>
