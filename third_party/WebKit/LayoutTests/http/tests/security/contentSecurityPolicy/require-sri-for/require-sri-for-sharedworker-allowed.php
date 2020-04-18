<!doctype html>
<script src="/resources/testharness.js"></script>
<script src="/resources/testharnessreport.js"></script>
<meta http-equiv="Content-Security-Policy" content="require-sri-for script; script-src 'self' 'unsafe-inline'">
<script>
    var executed_test = async_test('Test that a sharedworker can be created');
    try {
   		var worker = new SharedWorker("/security/contentSecurityPolicy/require-sri-for/sri-sharedworker.js");
   	} catch(e) {
   		executed_test.unreached_func("No errors should be thrown.");
   	}
   	document.addEventListener('securitypolicyviolation', executed_test.unreached_func("No report should be generated"));
	executed_test.done();
</script>

