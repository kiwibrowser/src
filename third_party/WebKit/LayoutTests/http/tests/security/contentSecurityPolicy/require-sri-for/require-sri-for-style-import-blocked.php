<?php
    header("Content-Security-Policy: require-sri-for style;");
?>
<!doctype html>
<style>
@import url("/security/contentSecurityPolicy/resources/style-set-red.css");
div {
    width: 100px;
    height: 100px;
}
</style>
<script src="/resources/testharness.js"></script>
<script src="/resources/testharnessreport.js"></script>
<script>
    
</script>
<script>
    async_test(t => {
        window.onload = t.step_func_done(_ => {
            assert_equals(document.styleSheets.length, 1);
            document.body.appendChild(document.createElement("p"));
            assert_equals(window.getComputedStyle(document.querySelector("p")).color, "rgb(0, 0, 0)");
        });
    }, "CSS imports without integrity do not load.");
</script>
