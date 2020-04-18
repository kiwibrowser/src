function selectRangeAfterLayoutAndPaint(startElement, startIndex, endElement, endIndex) {
    runAfterLayoutAndPaint(function() {
        selectRange(startElement, startIndex, endElement, endIndex);
      }, true);
}

function selectRange(startElement, startIndex, endElement, endIndex) {
  var range = document.createRange();
  range.setStart(startElement, startIndex);
  range.setEnd(endElement, endIndex);
  window.getSelection().addRange(range);
}

function selectNode(element) {
  var range = document.createRange();
  range.selectNode(element);
  window.getSelection().addRange(range);
}