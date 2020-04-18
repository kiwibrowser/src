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

embedder.test.assertFalse = function(condition) {
  if (condition) {
    LOG('Assertion failed: false != ' + condition);
    embedder.test.fail();
  }
};

// Tests begin.
function testExtensionViewCreationShouldSucceed(appToEmbed) {
  // Checking that there are no instances of <extensionview>.
  embedder.test.assertFalse(document.querySelector('extensionview'));
  var extensionview = new ExtensionView();

  // Appending new <extensionview> to DOM.
  document.body.appendChild(extensionview);

  // Checking that <extensionview> exists.
  if (document.querySelector('extensionview')) {
    embedder.test.succeed();
    return;
  }
  embedder.test.fail();
};

embedder.test.testList = {
  'testExtensionViewCreationShouldSucceed':
      testExtensionViewCreationShouldSucceed,
};

onload = function() {
  chrome.test.getConfig(function(config) {
    embedder.setUp_(config);
    chrome.test.sendMessage('Launched');
  });
};
