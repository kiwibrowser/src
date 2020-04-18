<?php
    header("Content-Security-Policy-Report-Only: require-sri-for script; script-src 'self' 'unsafe-inline'");
?>
<!doctype html>
<script src="/resources/testharness.js"></script>
<script src="/resources/testharnessreport.js"></script>
<script>
    async_test(t => {
        var watcher = new EventWatcher(t, document, ['securitypolicyviolation','securitypolicyviolation']);
        watcher
            .wait_for('securitypolicyviolation')
            .then(t.step_func_done(e => {
                assert_equals(e.blockedURI, "http://127.0.0.1:8000/resources/testharnessreport.js");
                return watcher.wait_for('securitypolicyviolation');
            }))
            .then(t.step_func_done(e => {
                assert_equals(e.blockedURI, "http://127.0.0.1:8000/security/contentSecurityPolicy/require-sri-for/ran.js");
            }));
    }, "Script without integrity generates reports.");

    var executed_test = async_test("Script that requires integrity executes and generates a violation report.");
</script>
<script src="ran.js"></script>
<script>
    assert_equals(z, 13);
    executed_test.done();
</script>
