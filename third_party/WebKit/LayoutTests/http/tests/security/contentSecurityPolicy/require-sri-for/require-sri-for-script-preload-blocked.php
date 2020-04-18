<!doctype html>
<head>
<script src="/resources/testharness.js"></script>
<script src="/resources/testharnessreport.js"></script>
<meta http-equiv="Content-Security-Policy" content="require-sri-for script; script-src 'self' 'unsafe-inline'">
<link rel="preload" href="not-ran.js" as="script">
</head>
<script>
	var executed_test = async_test("Script that requires integrity executes and does not generate a violation report.");
	document.addEventListener('securitypolicyviolation', executed_test.unreached_func("No report should be generated."));
   	var t = async_test('Makes sure that require-sri-for applies to preloaded resources.');
    window.addEventListener("load", t.step_func(function() {
        var entries = performance.getEntriesByType("resource");
        assert_equals(entries.length, 3);
        t.done();
    }));
    executed_test.done();
</script>
<script crossorigin integrity="sha384-SOGIJ0vOWzweNE6RLF/TOXGmPzCxF5+dNuBP4x1NgnKsfC4yFCVIDJILalTMwUrp" src="ran.js"></script>

