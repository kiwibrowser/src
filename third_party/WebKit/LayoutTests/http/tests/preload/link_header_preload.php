<?php
    header("Link: <http://127.0.0.1:8000/resources/square.png?background>;rel=preload;as=image", false);
    header("Link: <http://127.0.0.1:8000/resources/dummy.js>;rel=preload;as=script", false);
    header("Link: <http://127.0.0.1:8000/resources/dummy.css>;rel=preload;as=style", false);
    header("Link: <http://127.0.0.1:8000/resources/square.png>;rel=preload;as=image", false);
?>
<!DOCTYPE html>
<html>
<head></head>
<body>
<script src="/resources/testharness.js"></script>
<script src="/resources/testharnessreport.js"></script>
<script>
    var t = async_test('Makes sure that Link headers preload resources');
</script>
<script src="/resources/slow-script.pl?delay=200"></script>
<script>
    window.addEventListener("load", t.step_func(function() {
        var entries = performance.getEntriesByType("resource");
        assert_equals(entries.length, 7);
        t.done();
    }));
</script>
</body>
</html>
