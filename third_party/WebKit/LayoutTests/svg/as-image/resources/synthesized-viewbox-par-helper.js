'use strict';

function makeSvgImageUrl(width, height, viewBox, pAR) {
  var data = '<svg xmlns="http://www.w3.org/2000/svg"';
  data += ' width="' + width + '"';
  data += ' height="' + height + '"'
  if (viewBox)
    data += ' viewBox="' + viewBox + '"';
  if (pAR)
    data += ' preserveAspectRatio="' + pAR + '"';
  data += '><circle cx="100" cy="100" r="100" fill="blue"/></svg>';
  return 'data:image/svg+xml,' + encodeURIComponent(data);
}

function makeImage(config) {
  var img = new Image();
  img.src = makeSvgImageUrl(config.svgWidthAttr, config.svgHeightAttr);
  img.style.width = config.placeholderWidthAttr;
  img.style.height = config.placeholderHeightAttr;
  return img;
}

function makeReference(config) {
  var testData = new TestData(config);
  var intrinsic = testData.intrinsicInformation();
  var rect = testData.computeInlineReplacedSize();
  // Simplified for the case of percentages (assumes 100% in that case.)
  var viewBox = "0 0 " + (intrinsic.width || rect.width) + " " + (intrinsic.height || rect.height);
  var img = new Image();
  img.src = makeSvgImageUrl(rect.width, rect.height, viewBox, "none");
  img.style.width = rect.width;
  img.style.height = rect.height;
  return img;
}
