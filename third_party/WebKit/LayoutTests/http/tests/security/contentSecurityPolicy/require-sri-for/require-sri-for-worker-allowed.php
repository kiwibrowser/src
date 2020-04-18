<!doctype html>
<script src="/resources/testharness.js"></script>
<script src="/resources/testharnessreport.js"></script>
// TODO(mkwst) extend the policy when workers support SRI
<meta http-equiv="Content-Security-Policy" content="require-sri-for; script-src 'self' 'unsafe-inline'">
<script>
    var executed_test = async_test('Test that a worker can be created.');
    var worker = new Worker("/security/contentSecurityPolicy/require-sri-for/sri-worker.js");
    
    worker.onmessage = function(e){
        assert_equals(e.data, "ping");
    };
    document.addEventListener('securitypolicyviolation', executed_test.unreached_func("No report should be generated"));
    executed_test.done();
</script>
