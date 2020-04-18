function createSquareCompositedHighlight(node)
{
  return _createHighlight(node, "squaredHighlight highlightDiv composited");
}

function createCompositedHighlight(node)
{
  return _createHighlight(node, "highlightDiv composited");
}

function createHighlight(node)
{
  return _createHighlight(node, "highlightDiv");
}

function _createHighlight(node, classes) {
  var div = document.createElement('div');
  div.setAttribute('id', 'highlight');
  div.setAttribute('class', classes);
  document.body.appendChild(div);

  var clientRect = node.getBoundingClientRect();
  div.style.top = clientRect.top + "px";
  div.style.left = clientRect.left + "px";
  div.style.width = clientRect.width + "px";
  div.style.height = clientRect.height + "px";

  return div;
}

function useMockHighlight() {
  if (window.internals)
    internals.settings.setMockGestureTapHighlightsEnabled(true);
}

function testHighlightTarget(id) {
    useMockHighlight();

    var clientRect = document.getElementById('highlightTarget').getBoundingClientRect();
    x = (clientRect.left + clientRect.right) / 2;
    y = (clientRect.top + clientRect.bottom) / 2;
    if (window.testRunner) {
        testRunner.dumpAsTextWithPixelResults();
        testRunner.waitUntilDone();
    }

    if (window.eventSender) {
        eventSender.gestureTapDown(x, y, 30, 30);
        eventSender.gestureShowPress(x, y, 30, 30);
        window.setTimeout(function() { testRunner.notifyDone(); }, 0);
    } else {
        debug("This test requires eventSender");
    }
}
