<?php
    header("Link: <http://127.0.0.1:8000/resources/square.png?background>;rel=preload;as=image", false);
    header("Link: <http://127.0.0.1:8000/resources/dummy.js>;rel=preload;as=script", false);
    header("Link: <http://127.0.0.1:8000/resources/dummy.css>;rel=preload;as=style", false);
    header("Link: <http://127.0.0.1:8000/resources/square.png>;rel=preload;as=image", false);
?>
<!DOCTYPE html>
<script src="/resources/testharness.js"></script>
<script src="/resources/testharnessreport.js"></script>
<script>
    var t = async_test('Makes sure that Link headers preload resources');
</script>
<script src="/resources/slow-script.pl?delay=200"></script>
<style>
    #background {
        width: 200px;
        height: 200px;
        background-image: url(/resources/square.png?background);
    }
</style>
<link rel="stylesheet" href="/resources/dummy.css">
<script src="/resources/dummy.js"></script>
<div id="background"></div>
<script>
    document.write('<img src="/resources/square.png">');
    window.addEventListener("load", t.step_func(function() {
        var entries = performance.getEntriesByType("resource");
        var found_background_first = false;
        for (var i = 0; i < entries.length; ++i) {
            var entry = entries[i];
            if (entry.name.indexOf("square") != -1) {
                if (entry.name.indexOf("background") != -1)
                    found_background_first = true;
                //break;
            }
            console.log(entry.name);
        }
        assert_true(found_background_first);
        assert_equals(entries.length, 7);
        t.done();
    }));
</script>
