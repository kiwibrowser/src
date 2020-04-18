<?php
    header("Content-Security-Policy: require-sri-for style;");
?>
<!doctype html>
<script src="/resources/testharness.js"></script>
<script src="/resources/testharnessreport.js"></script>
<script>
	var executed_test = async_test("Script that requires integrity executes and does not generate a violation report.");
	document.addEventListener('securitypolicyviolation', executed_test.unreached_func("No report should be generated."));
  	var t = async_test('Makes sure that require-sri-for applies to preloaded resources.');
    window.addEventListener("load", t.step_func(function() {
        var entries = performance.getEntriesByType("resource");
        assert_equals(entries.length, 2);
        t.done();
    }));
    executed_test.done();
</script>
<script>
    async_test(t => {
        window.onload = t.step_func_done(_ => {
            assert_equals(document.styleSheets.length, 0);
        });
    }, "Stylesheets without integrity do not load.");
</script>
<link rel="preload" href="/security/contentSecurityPolicy/resources/style-set-red.css" as="style">
