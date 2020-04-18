<!doctype html>
<script src="/resources/testharness.js"></script>
<script src="/resources/testharnessreport.js"></script>
<meta http-equiv="Content-Security-Policy" content="require-sri-for script; script-src 'self' 'unsafe-inline'">
<script>
	test(function () {
    	assert_throws("SecurityError",
    		function() {
    			new SharedWorker("http://127.0.0.1:8000/security/contentSecurityPolicy/require-sri-for/sri-sharedworker.js");
    		},
        	"The worker creation was blocked");
	}, "Checks that shared worker creation is blocked");
</script>
