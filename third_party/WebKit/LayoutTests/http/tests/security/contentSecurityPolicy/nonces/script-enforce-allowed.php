<?php
    header("Content-Security-Policy: script-src 'self' 'nonce-abc'");
?>
<!doctype html>
<script src="/resources/testharness.js"></script>
<script src="/resources/testharnessreport.js"></script>
<script nonce="abc">
    var executed_test = async_test("Nonced script executes, and does not generate a violation report.");
    document.addEventListener('securitypolicyviolation', executed_test.unreached_func("No report should be generated"));
</script>
<script nonce="abc">
    executed_test.done();
</script>
