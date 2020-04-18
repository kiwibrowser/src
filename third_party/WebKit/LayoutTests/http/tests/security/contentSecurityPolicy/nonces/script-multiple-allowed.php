<?php
    header("Content-Security-Policy: script-src 'unsafe-inline' 'self'");
    header("Content-Security-Policy-Report-Only: script-src 'self' 'nonce-abc'");
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
                assert_equals(e.lineNumber, 29);
                return watcher.wait_for('securitypolicyviolation');
            }))
            .then(t.step_func_done(e => {
                assert_equals(e.blockedURI, "inline");
                assert_equals(e.lineNumber, 32);
            }));
    }, "Unnonced script blocks generate reports.");

    var testList = [
        async_test("Script without nonce executes"),
        async_test("Script with incorrect nonce executes")
    ];
    var executed = [
        false,
        false
    ];
</script>
<script>
    executed[0] = true;
</script>
<script nonce="xyz">
    executed[1] = true;
</script>
<script nonce="abc">
    testList[0].step(_ => {
        assert_true(executed[0]);
        testList[0].done();
    });

    testList[1].step(_ => {
        assert_true(executed[1]);
        testList[1].done();
    });
</script>
