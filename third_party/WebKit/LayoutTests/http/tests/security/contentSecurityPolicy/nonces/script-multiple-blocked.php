<?php
    header("Content-Security-Policy: script-src 'self' 'nonce-abc'");
    header("Content-Security-Policy-Report-Only: script-src 'unsafe-inline' 'self'");
?>
<!doctype html>
<script src="/resources/testharness.js"></script>
<script src="/resources/testharnessreport.js"></script>
<script nonce="abc">
    async_test(t => {
        var watcher = new EventWatcher(t, document, ['securitypolicyviolation','securitypolicyviolation']);
        watcher
            .wait_for('securitypolicyviolation')
            .then(t.step_func(e => {
                assert_equals(e.blockedURI, "inline");
                assert_equals(e.lineNumber, 23);
                return watcher.wait_for('securitypolicyviolation');
            }))
            .then(t.step_func_done(e => {
                assert_equals(e.blockedURI, "inline");
                assert_equals(e.lineNumber, 26);
            }));
    }, "Unnonced script blocks generate reports.");

    var executed_test = async_test("Nonced script executes, and does not generate a violation report.");
    var unexecuted_test = async_test("Blocks without correct nonce do not execute, and generate violation reports");
</script>
<script>
    unexecuted_test.assert_unreached("This code block should not execute.");
</script>
<script nonce="xyz">
    unexecuted_test.assert_unreached("This code block should not execute.");
</script>
<script nonce="abc">
    executed_test.done();
    unexecuted_test.done();
</script>
