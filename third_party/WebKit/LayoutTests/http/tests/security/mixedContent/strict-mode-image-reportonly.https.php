<?php
  header("Content-Security-Policy-Report-Only: block-all-mixed-content");
?>
<!doctype html>
<script src="/resources/testharness.js"></script>
<script src="/resources/testharnessreport.js"></script>
<body>
<script>
    function waitForEvent(t, el, name) {
        return new EventWatcher(t, el, [name]).wait_for(name);
    }

    async_test(t => {
        var i = document.createElement('img');
        i.onerror = t.unreached_func();

        Promise.all([
            waitForEvent(t, i, 'load').then(_ => {
                assert_equals(128, i.naturalWidth);
                assert_equals(128, i.naturalHeight);
            }),
            waitForEvent(t, document, 'securitypolicyviolation').then(e => {
                var expectations = {
                    'documentURI': document.location.toString(),
                    'referrer': document.referrer,
                    'blockedURI': 'http://example.test:8080/security/resources/compass.jpg?t=1',
                    'violatedDirective': 'block-all-mixed-content',
                    'effectiveDirective': 'block-all-mixed-content',
                    'originalPolicy': 'block-all-mixed-content',
                    'disposition': 'report',
                    'sourceFile': '',
                    'lineNumber': 0,
                    'columnNumber': 0,
                    'statusCode': 0
                };
                for (key in expectations)
                    assert_equals(expectations[key], e[key], key);
            })
        ]).then(t.step_func_done());

        i.src = "http://example.test:8080/security/resources/compass.jpg?t=1";
    }, "Mixed images are allowed and generate CSP violation reports in the presence of 'block-all-mixed-content' in report-only mode.");
</script>
