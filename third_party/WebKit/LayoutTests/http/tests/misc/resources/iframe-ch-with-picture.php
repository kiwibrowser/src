<?php
    header("ACCEPT-CH: DPR, Width, Viewport-Width");
?>
<!DOCTYPE html>
<body>
<script>
    var fail = function(num) {
        parent.postMessage("fail "+ num, "*");
    };

    var success = function() {
        parent.postMessage("success", "*");
    };

    var resize = function() {
        parent.postMessage("resize", "*");
    };

    var counter = 1;
    var error = function() {
        fail(counter);
    }
    var load = function() {
        if (counter == 1) {
            ++counter;
            resize();
            return;
        }
        success();
    }
</script>
<picture>
    <source sizes="50vw" media="(min-width: 800px)" srcset="image-checks-for-width.php?rw=400">
    <source sizes="50vw" srcset="image-checks-for-width.php?rw=300">
    <img onerror="error()" onload="load()">
</picture>
