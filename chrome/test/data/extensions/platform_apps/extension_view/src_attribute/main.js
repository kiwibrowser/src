// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var LOG = function(msg) {
  window.console.log(msg);
};

var embedder = {};
embedder.setUp_ = function(config) {
  if (!config || !config.testServer)
    return;
};

window.runTest = function(testName, appToEmbed) {
  if (!embedder.test.testList[testName]) {
    LOG('Incorrect testName: ' + testName);
    embedder.test.fail();
    return;
  }

  // Run the test.
  embedder.test.testList[testName](appToEmbed);
};

embedder.test = {};
embedder.test.succeed = function() {
  chrome.test.sendMessage('TEST_PASSED');
};

embedder.test.fail = function() {
  chrome.test.sendMessage('TEST_FAILED');
};

embedder.test.assertEq = function(a, b) {
  if (a != b) {
    LOG('assertion failed: ' + a + ' != ' + b);
    embedder.test.fail();
  }
};

var checkSrcAttribute = function(element, expectedValue) {
  embedder.test.assertEq(expectedValue, element.src);
};

// Tests begin.
function testSrcAttribute(extensionId) {
  var srcOne = 'data:text/html,<body>One</body>';
  var srcTwo = 'data:text/html,<body>Two</body>';
  var fullUrlOne = 'chrome-extension://' + extensionId + '/' + srcOne;
  var fullUrlTwo = 'chrome-extension://' + extensionId + '/' + srcTwo;

  var extensionview = document.querySelector('extensionview');

  // Load a URL to <extensionview>.
  extensionview.load(fullUrlOne)
  .then(function onLoadResolved() {
    // Check that the src attribute has been set.
    checkSrcAttribute(extensionview, fullUrlOne);

    // Set the src attribute using setAttribute.
    extensionview.setAttribute('src', fullUrlTwo);
    // Check that the src attribute was not updated.
    checkSrcAttribute(extensionview, fullUrlOne);
    embedder.test.succeed();
  }, function onLoadRejected() {
    embedder.test.fail();
  });
};

embedder.test.testList = {
  'testSrcAttribute': testSrcAttribute,
};

onload = function() {
  chrome.test.getConfig(function(config) {
    embedder.setUp_(config);
    chrome.test.sendMessage('Launched');
  });
};
