<?php
header("Content-Type: text/html;charset=utf-8", false);
header("Link: <http://127.0.0.1:8000/preload/resources/charset.js>;rel=preload;as=script", false);
?>
<script src="../resources/testharness.js"></script>
<script src="../resources/testharnessreport.js"></script>
<script>
    var t = async_test("Makes sure that header preloaded resources are downloaded with the document's charset");
    window.addEventListener("load", t.step_func(function() {
        assert_equals(window.preload_header, 'Кириллиця');
        t.done();
    }));
</script>
<script src="resources/charset.js"></script>
