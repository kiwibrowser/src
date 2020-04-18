<?php
    header("Content-Security-Policy: script-src 'unsafe-inline' 'self'");
    header("Content-Security-Policy-Report-Only: script-src 'nonce-abc'");
?>
<!doctype html>
<script nonce="abc" src="/resources/testharness.js"></script>
<script nonce="abc" src="/resources/testharnessreport.js"></script>
<script nonce="abc">
    async_test(t => {
        document.addEventListener('securitypolicyviolation', t.unreached_func("No report should be generated"));
        window.onload = t.step_func(_ => {
            var link = document.createElement('link');
            link.setAttribute("rel", "import");
            link.setAttribute("nonce", "abc");
            link.setAttribute("href", "/security/resources/blank.html");

            link.onerror = t.unreached_func("The import should load.");
            link.onload = t.step_func_done(_ => {
              assert_true(!!link.import);
            });

            document.body.appendChild(link);
        });
    }, "Nonced imports load, and do not trigger reports.");
</script>
