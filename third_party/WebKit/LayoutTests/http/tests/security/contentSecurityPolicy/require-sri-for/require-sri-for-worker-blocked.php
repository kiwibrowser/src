<!doctype html>
<script src="/resources/testharness.js"></script>
<script src="/resources/testharnessreport.js"></script>
<meta http-equiv="Content-Security-Policy" content="require-sri-for script; script-src 'self' 'unsafe-inline'">
<script>
    var executed_test = async_test('Test that a worker can be created, but execution is blocked.');
    try {
    	var worker = new Worker("/security/contentSecurityPolicy/require-sri-for/sri-worker.js");
        worker.onmessage = function(e){
            executed_test.assert_unreached("This code block should not execute.");
        };
    } catch (e) {
    	assert_equals(e.message, "Failed to construct 'Worker': Access to the script at 'http://127.0.0.1:8000/security/contentSecurityPolicy/require-sri-for/sri-worker.js' is denied by the document's Content Security Policy.");
    }
    document.addEventListener('securitypolicyviolation', executed_test.unreached_func("No report should be generated."));
    executed_test.done();
</script>
