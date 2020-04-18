<?php
    header("Link: <http://127.0.0.1:8000/resources/dummy.js>;rel=preload;as=script", false);
?>
<html>
<head>
<script>
function scheduleScriptLoad() {
    window.setTimeout(loadScript, 0);
}

function loadScript() {
    var script = document.createElement("script");
    script.type = "text/javascript";
    script.src = "http://127.0.0.1:8000/resources/dummy.js";
    script.onload = function() {
        setTimeout(scriptLoaded, 0);
    };
    document.head.appendChild(script);
}

function scriptLoaded() {
    var resources = performance.getEntriesByType("resource");
    var dummies = 0;
    for (var i = 0; i < resources.length; ++i) {
        if (resources[i].name.indexOf("dummy.js") != -1)
            ++dummies;
    }

    if (dummies == 1)
        console.log("PASS - 1 resource loaded");
    else
        console.log("FAIL - " + dummies + " resources loaded");
}
</script>
</head>
</html>

