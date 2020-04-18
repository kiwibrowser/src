// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var LOG = function(msg) {
  window.console.log(msg);
};

var embedder = {};
embedder.setUp_ = function(config) {
  if (!config || !config.testServer)
    return;0
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

var checkExtensionAttribute = function(element, expectedValue) {
  embedder.test.assertEq(expectedValue, element.extension);
};

// Tests begin.
function testExtensionAttribute(extensionId) {
  var secondExtensionId = 'secondExtensionId';
  var src = 'data:text/html,<body>One</body>';
  var fullUrl = 'chrome-extension://' + extensionId + '/' + src;

  var extensionview = document.querySelector('extensionview');
  // Load a URL to <extensionview>.
  extensionview.load(fullUrl)
  .then(function onLoadResolved() {
    // Check that the extension attribute has been set.
    checkExtensionAttribute(extensionview, extensionId);

    // Set the extension attribute using setAttribute.
    extensionview.setAttribute('extension', secondExtensionId);
    // Check that the extension attribute was not updated.
    checkExtensionAttribute(extensionview, extensionId);
    embedder.test.succeed();
  }, function onLoadRejected() {
    embedder.test.fail();
  });
};

embedder.test.testList = {
  'testExtensionAttribute': testExtensionAttribute,
};

onload = function() {
  chrome.test.getConfig(function(config) {
    embedder.setUp_(config);
    chrome.test.sendMessage('Launched');
  });
};
