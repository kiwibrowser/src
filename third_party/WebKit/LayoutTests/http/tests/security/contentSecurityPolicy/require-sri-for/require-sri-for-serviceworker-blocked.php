<!doctype html>
<script src="/resources/testharness.js"></script>
<script src="/resources/testharnessreport.js"></script>
<meta http-equiv="Content-Security-Policy" content="require-sri-for script; script-src 'self' 'unsafe-inline'">
<script>
    var executed_test = async_test('Test that a service worker can not be registered.');
   	navigator.serviceWorker.register("resources/service-worker.js").then(function (registration) {
    }).catch(function (error) {
        executed_test.done();
    });
    document.addEventListener('securitypolicyviolation', executed_test.unreached_func("No report should be generated"));
</script>
