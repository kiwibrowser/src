function nodeToString(node) {
  var str = '';
  if (node.nodeType == Node.ELEMENT_NODE) {
    str += node.nodeName;
    if (node.id)
      str += '#' + node.id;
    else if (node.class)
      str += '.' + node.class;
  } else if (node.nodeType == Node.TEXT_NODE) {
    str += '\'' + node.data + '\'';
  } else if (node.nodeType == Node.DOCUMENT_NODE) {
    str += '#document';
  }
  return str;
}

function nodeListToString(nodes) {
  var nodeString = '';

  for (var i = 0; i < nodes.length; i++) {
    var str = nodeToString(nodes[i]);
    if (!str)
      continue;
    nodeString += str;
    if (i + 1 < nodes.length)
      nodeString += ', ';
  }
  return nodeString;
}

function assertElementsFromPoint(actual, expected) {
  shouldBeEqualToString('nodeListToString(' + actual + ')', nodeListToString(expected));
}

function assertElementInSequence(sequence, value, expectedInSequence) {
  if (expectedInSequence)
    shouldBeGreaterThanOrEqual(sequence + '.indexOf(' + value + ')', '0');
  else
    shouldBeEqualToNumber(sequence + '.indexOf(' + value + ')', -1);
}

function checkElementsFromPointFourCorners(doc, evalElement) {
  var element = eval(evalElement);
  var shouldReceivePointerEvents = window.getComputedStyle(element).pointerEvents != 'none';
  var rect = element.getBoundingClientRect();

  var topLeft = {x: rect.left + 1, y: rect.top + 1};
  var topRight = {x: rect.right - 1, y: rect.top + 1};
  var bottomLeft = {x: rect.left + 1, y: rect.bottom - 1};
  var bottomRight = {x: rect.right - 1, y: rect.bottom - 1};

  var topLeftElements = doc + '.elementsFromPoint(' + topLeft.x + ', ' + topLeft.y + ')';
  var topRightElements = doc + '.elementsFromPoint(' + topRight.x + ', ' + topRight.y + ')';
  var bottomLeftElements = doc + '.elementsFromPoint(' + bottomLeft.x + ', ' + bottomLeft.y + ')';
  var bottomRightElements = doc + '.elementsFromPoint(' + bottomRight.x + ', ' + bottomRight.y + ')';

  assertElementInSequence(topLeftElements, evalElement, shouldReceivePointerEvents);
  assertAllElementsIntersectPoint(topLeftElements, topLeft);
  assertElementInSequence(topRightElements, evalElement, shouldReceivePointerEvents);
  assertAllElementsIntersectPoint(topRightElements, topRight);
  assertElementInSequence(bottomLeftElements, evalElement, shouldReceivePointerEvents);
  assertAllElementsIntersectPoint(bottomLeftElements, bottomLeft);
  assertElementInSequence(bottomRightElements, evalElement, shouldReceivePointerEvents);
  assertAllElementsIntersectPoint(bottomRightElements, bottomRight);
}

function assertAllElementsIntersectPoint(sequence, point) {
  var elementsEv;
  try {
    elementsEv = eval(sequence);
  } catch (e) {
    testFailed('Evaluating ' + sequence + ': Threw exception ' + e);
    return;
  }

  var numElements = elementsEv.length;
  for (var i = 0; i < numElements; i++) {
    var clientRect = elementsEv[i].getBoundingClientRect();
    if (point.x < clientRect.left
        || point.x > clientRect.right
        || point.y < clientRect.top
        || point.y > clientRect.bottom) {
      testFailed(sequence + '[' + i + '].getBoundingClientRect() does not intersect (' + point.x + ', ' + point.y + ')');
      return;
    }
  }
  testPassed('All elements in ' + sequence + ' intersect (' + point.x + ', ' + point.y + ')');
}
