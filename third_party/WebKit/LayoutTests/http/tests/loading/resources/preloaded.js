var results=[];

if (window.testRunner) {
    testRunner.dumpAsText();
}

function checkForPreload(url, shouldbe) {
    var preloaded = internals.isPreloaded(url);
    if ((preloaded && shouldbe) || (!preloaded && !shouldbe))
        results.push("PASS " + url);
    else
        results.push("FAIL " + url);
}

function printPreloadResults(){
    for(var i = 0; i < results.length; i++) {
        document.body.appendChild(document.createElement("br"));
        document.body.appendChild(document.createTextNode(results[i]));
    }
}
