<!doctype html>
<script src="/resources/testharness.js"></script>
<script src="/resources/testharnessreport.js"></script>
<meta http-equiv="Content-Security-Policy" content="require-sri-for script; script-src 'self' 'unsafe-inline'">
<script>
    async_test(t => {
        var watcher = new EventWatcher(t, document, ['securitypolicyviolation']);
        watcher
            .wait_for('securitypolicyviolation')
            .then(t.step_func_done(e => {
                assert_equals(e.blockedURI, "http://127.0.0.1:8000/security/contentSecurityPolicy/require-sri-for/not-ran.js");
            }));
    }, "Script without integrity generates reports.");

    var executed_test = async_test("Script that requires integrity executes and does not generate a violation report.");
    var unexecuted_test = async_test("Request to script without integrity is blocked, and generates violation report");
</script>
<script crossorigin integrity="sha384-SOGIJ0vOWzweNE6RLF/TOXGmPzCxF5+dNuBP4x1NgnKsfC4yFCVIDJILalTMwUrp" src="ran.js"></script>
<script src="not-ran.js"></script>
<script>
    executed_test.done();
    unexecuted_test.done();
</script>
