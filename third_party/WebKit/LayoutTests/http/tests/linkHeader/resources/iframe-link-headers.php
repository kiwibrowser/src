<?php
    header("Link: <http://127.0.0.1:8000/resources/square.png>;rel=preload;as=image;", false);
?>
<!DOCTYPE html>
<!-- Test that
    (1) the URL specified in the link header is preloaded, and
    (2) isPreloaded() returns true before, in, and after document's load event.
-->
<script>
function test() {
    if (window.internals) {
        if (internals.isPreloaded("http://127.0.0.1:8000/resources/square.png"))
            top.postMessage("squareloaded", "*");
        else
            top.postMessage("notloaded", "*");
    }
}
function onLoad() {
    test();
    setTimeout(test, 100);
}
test();
</script>
<body onload="onLoad()">
</body>
