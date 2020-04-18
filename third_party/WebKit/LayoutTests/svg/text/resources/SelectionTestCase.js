if (!window.eventSender || !window.testRunner) {
    alert('This test needs to be run in DRT, to get results!');
}

var svgRoot = 0;

// Map 'point' into absolute coordinates, usable for eventSender
function transformPoint(point, matrix) {
    return point.matrixTransform(matrix);
}

function transformRect(rect, matrix) {
    var topLeft = svgRoot.createSVGPoint();
    topLeft.x = rect.x;
    topLeft.y = rect.y;
    topLeft = transformPoint(topLeft, matrix);

    var bottomRight = svgRoot.createSVGPoint();
    bottomRight.x = rect.x + rect.width;
    bottomRight.y = rect.y + rect.height;
    bottomRight = transformPoint(bottomRight, matrix);

    var newRect = svgRoot.createSVGRect();
    newRect.x = topLeft.x;
    newRect.y = topLeft.y;
    newRect.width = bottomRight.x - topLeft.x;
    newRect.height = bottomRight.y - topLeft.y;
    return newRect;
}

function toAbsoluteCoordinates(point, element) {
    // getScreenCTM() returns the transformation matrix from current user units (i.e., after application of the ‘transform’ property)
    // to the parent user agent's notice of a "pixel".
    return transformPoint(point, element.getScreenCTM());
}

// Select a range of characters in text element 'id', from the start position of the 'start' character to the end position of the 'end' character
function selectRange(id, start, end, expectedText) {
    var element = document.getElementById(id);
    var startExtent = element.getExtentOfChar(start);
    var endExtent = element.getExtentOfChar(end);

    var startPos = element.getStartPositionOfChar(start);
    var endPos = element.getEndPositionOfChar(end);

    // Handle lengthAdjust+textLength on our own.
    var scale = element.textLength.baseVal.value / element.getComputedTextLength();
    if (element.lengthAdjust.baseVal == SVGTextContentElement.LENGTHADJUST_SPACINGANDGLYPHS && scale != 1) {
        svgRoot = element.ownerSVGElement;

        var firstCharPosition = element.getStartPositionOfChar(0);
        var scaleMatrix = svgRoot.createSVGMatrix()
                          .translate(firstCharPosition.x, firstCharPosition.y)
                          .scaleNonUniform(scale, 1)
                          .translate(-firstCharPosition.x, -firstCharPosition.y);

        startPos = transformPoint(startPos, scaleMatrix);
        endPos = transformPoint(endPos, scaleMatrix);

        startExtent = transformRect(startExtent, scaleMatrix);
        endExtent = transformRect(endExtent, scaleMatrix);
    }

    if (window.eventSender) {
        // Trigger 'partial glyph selection' code, by adjusting the end x position by half glyph width
        var xOldStart = startPos.x;
        var xOldEnd = endPos.x;
        endPos.x -= endExtent.width / 2 - 1;

        // Round the points "inwards" to avoid being affected by the truncation
        // taking place in eventSender.mouseMoveTo(...).
        startPos.x = Math.ceil(startPos.x);

        var absStartPos = toAbsoluteCoordinates(startPos, element);
        var absEndPos = toAbsoluteCoordinates(endPos, element);

        // Move to selection origin and hold down mouse
        eventSender.mouseMoveTo(absStartPos.x, absStartPos.y);
        eventSender.mouseDown();

        // Move again to start selection
        eventSender.mouseMoveTo(absStartPos.x, absStartPos.y);

        // Move to end location and release mouse
        eventSender.mouseMoveTo(absEndPos.x, absEndPos.y);
        eventSender.mouseUp();

        startPos.x = xOldStart;
        endPos.x = xOldEnd;
    }

    // Mark start position using a green line
    var startLineElement = document.createElementNS("http://www.w3.org/2000/svg", "svg:line");
    startLineElement.setAttribute("x1", startPos.x);
    startLineElement.setAttribute("y1", startExtent.y);
    startLineElement.setAttribute("x2", startPos.x);
    startLineElement.setAttribute("y2", startExtent.height + 1);
    startLineElement.setAttribute("stroke", "green");
    document.getElementById("container").appendChild(startLineElement);

    // Mark end position using a green line
    var endLineElement = document.createElementNS("http://www.w3.org/2000/svg", "svg:line");
    endLineElement.setAttribute("x1", endPos.x);
    endLineElement.setAttribute("y1", endExtent.y);
    endLineElement.setAttribute("x2", endPos.x);
    endLineElement.setAttribute("y2", endExtent.height + 1);
    endLineElement.setAttribute("stroke", "green");
    document.getElementById("container").appendChild(endLineElement);

    // Highlight rect that we've selected using the extent information
    var rectElement = document.createElementNS("http://www.w3.org/2000/svg", "svg:rect");
    rectElement.setAttribute("x", startExtent.x);
    rectElement.setAttribute("y", endExtent.y);
    rectElement.setAttribute("width", endExtent.x + endExtent.width - startExtent.x);
    rectElement.setAttribute("height", endExtent.height);
    rectElement.setAttribute("fill-opacity", "0.4");
    rectElement.setAttribute("fill", "red");
    document.getElementById("container").appendChild(rectElement);

    // Check selection worked properly, otherwhise report error
    var actualText = window.getSelection().toString();
    if (actualText != expectedText) {
        var textElement = document.createElementNS("http://www.w3.org/2000/svg", "svg:text");
        textElement.setAttribute("x", "0");
        textElement.setAttribute("y", "35");
        textElement.setAttribute("fill", "red");
        textElement.setAttribute("transform", "scale(0.5)");
        textElement.setAttribute("font-size", "8");
        textElement.textContent = "Expected '" + expectedText + "' to be selected, got: '" + actualText + "'";
        document.getElementById("container").appendChild(textElement);
    }
}

function selectTextFromCharToPoint(selectionInfo, mouse, expected) {
  var element = document.getElementById(selectionInfo.id);
  var startPos = element.getStartPositionOfChar(selectionInfo.offset);
  var absStartPos = toAbsoluteCoordinates(startPos, element);
  if (window.eventSender) {
    eventSender.mouseMoveTo(absStartPos.x, absStartPos.y);
    eventSender.mouseDown();
    eventSender.mouseMoveTo(mouse.x, mouse.y);
    eventSender.mouseUp();
  }

  selection = window.getSelection();
  startElementId = selection.anchorNode.parentElement.id;
  endElementId = selection.focusNode.parentElement.id;
  // TODO(shanmuga.m): It'd be preferable to have the assertions in the actual test-files.
  assert_equals(startElementId, expected.startElementId);
  assert_equals(selection.anchorOffset, expected.start);
  assert_equals(endElementId, expected.endElementId);
  assert_equals(selection.focusOffset, expected.end);
  if (window.eventSender) {
    eventSender.mouseMoveTo(0,0);
    eventSender.mouseDown();
    eventSender.mouseUp();
  }
}

function getEndPosition(id, offset, gap) {
  var element = document.getElementById(id);
  var endPos = element.getEndPositionOfChar(offset);
  endPos.x += gap.x;
  endPos.y += gap.y;
  var absEndPos = toAbsoluteCoordinates(endPos, element);
  return absEndPos;
}