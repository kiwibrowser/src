function listener() {
}

function log(msg) {
    var span = document.createElement("span");
    document.getElementById("console").appendChild(span);
    span.innerHTML = msg + '<br />';
}

function nameForNode(node) {
    if (!node)
        return "[unknown-node]";
    var name = node.nodeName;
    if (node.id)
        name += '#' + node.id;
   return name;
}

function sortRects(a, b) {
    return a.layerRelativeRect.top - b.layerRelativeRect.top
        || a.layerRelativeRect.left - b.layerRelativeRect.left
        || a.layerRelativeRect.width - b.layerRelativeRect.width
        || a.layerRelativeRect.height - b.layerRelativeRect.right
        || nameForNode(a.layerAssociatedNode).localeCompare(nameForNode(b.layerAssociatedNode))
        || a.layerType.localeCompare(b.layerType);
}

var preRunHandlerForTest = {};

function testElement(element) {
    element.addEventListener('touchstart', listener, {passive: false});

    // Run any test-specific handler AFTER adding the touch event listener
    // (which itself causes rects to be recomputed).
    if (element.id in preRunHandlerForTest)
        preRunHandlerForTest[element.id](element);

    if (window.internals)
        internals.forceCompositingUpdate(document);

    logRects(element.id);

    // If we're running manually, leave the handlers in place so the user
    // can use dev tools 'show potential scroll bottlenecks' for visualization.
    if (window.internals)
        element.removeEventListener('touchstart', listener, false);
}

function logRects(testName, opt_noOverlay) {
    if (!window.internals) {
        log(testName + ': not run');
        return;
    }

    var rects = internals.touchEventTargetLayerRects(document);
    if (rects.length == 0)
        log(testName + ': no rects');

    var sortedRects = new Array();
    for ( var i = 0; i < rects.length; ++i)
        sortedRects[i] = rects[i];
    sortedRects.sort(sortRects);
    for ( var i = 0; i < sortedRects.length; ++i) {
        var node = sortedRects[i].layerAssociatedNode;
        var r = sortedRects[i].layerRelativeRect;
        var nameSuffix = "";
        if (sortedRects[i].layerType)
            nameSuffix += " " + sortedRects[i].layerType;
        var offsetX = sortedRects[i].associatedNodeOffsetX;
        var offsetY = sortedRects[i].associatedNodeOffsetY;
        if (offsetX || offsetY)
            nameSuffix += "[" + offsetX + "," + offsetY + "]"
        log(testName + ": " + nameForNode(node) + nameSuffix + " ("
            + r.left + ", " + r.top + ", " + r.width + ", " + r.height + ")");

        if (visualize && node && !opt_noOverlay && window.location.hash != '#nooverlay') {
            var patch = document.createElement("div");
            patch.className = "overlay generated display-when-done";
            patch.style.left = r.left + "px";
            patch.style.top = r.top + "px";
            patch.style.width = r.width + "px";
            patch.style.height = r.height + "px";

            if (node === document) {
                patch.style.position = "absolute";
                document.body.appendChild(patch);
            } else {
                // Use a zero-size container to avoid changing the position of
                // the existing elements.
                var container = document.createElement("div");
                container.className = "overlay-container generated";
                patch.style.position = "relative";
                node.appendChild(container);
                var x = -offsetX;
                var y = -offsetY;
                if (container.offsetParent != node) {
                    // Assume container.offsetParent == node.offsetParent
                    y += node.offsetTop - container.offsetTop;
                    x += node.offsetLeft - container.offsetLeft;
                }
                if (x || y) {
                    container.style.top = y + "px";
                    container.style.left = x + "px";
                }
                container.classList.add("display-when-done");
                container.appendChild(patch);
            }
        }
    }

    log('');
}

// Set this to true in order to visualize the results in an image.
// Elements that are expected to be included in hit rects have a red border.
// The actual hit rects are in a green tranlucent overlay.
var visualize = false;

if (window.testRunner) {
    if (visualize)
        testRunner.dumpAsTextWithPixelResults();
    else
        testRunner.dumpAsText();
    document.documentElement.setAttribute('dumpRenderTree', 'true');
} else {
    // Note, this test can be run interactively in content-shell with
    // --expose-internals-for-testing.  In that case we almost certainly
    // want to visualize the results.
    visualize = true;
}

if (window.internals) {
    internals.settings.setMockScrollbarsEnabled(true);
}

window.onload = function() {
    // Run each general test case.
    var tests = document.querySelectorAll('.testcase');

    // Add document wide touchend and touchcancel listeners and ensure the
    // listeners do not affect compositor hit test rects.
    document.documentElement.addEventListener('touchend', listener, false);
    document.documentElement.addEventListener('touchcancel', listener, false);

    for ( var i = 0; i < tests.length; i++) {
        // Force a compositing update before testing each case to ensure that
        // any subsequent touch rect updates are actually done because of
        // the event handler changes in the test itself.
        if (window.internals)
            internals.forceCompositingUpdate(document);
        testElement(tests[i]);
    }

    if (window.additionalTests)
        additionalTests();

    if (!visualize && window.internals) {
        var testContainer = document.getElementById("tests");
        testContainer.parentNode.removeChild(testContainer);
    }

    document.documentElement.setAttribute('done', 'true');
};
