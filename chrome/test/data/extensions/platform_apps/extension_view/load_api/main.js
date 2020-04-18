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

window.runTest = function(testName, appToEmbed, secondAppToEmbed) {
  if (!embedder.test.testList[testName]) {
    LOG('Incorrect testName: ' + testName);
    embedder.test.fail();
    return;
  }

  // Run the test.
  embedder.test.testList[testName](appToEmbed, secondAppToEmbed);
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

var checkSrcAttribute = function(element, expectedValue) {
  embedder.test.assertEq(expectedValue, element.src);
};

var extensionScheme = 'chrome-extension://';
var srcOne = 'data:text/html,<body>One</body>';
var srcTwo = 'data:text/html,<body>Two</body>';

// Tests begin.

// Call load with a specified extension ID and src.
function testLoadAPIFunction(extensionId) {
  var extensionview = document.querySelector('extensionview');

  extensionview.load(extensionScheme + extensionId + '/' + srcOne)
      .then(function() {
        checkExtensionAttribute(extensionview, extensionId);
        checkSrcAttribute(
            extensionview, extensionScheme + extensionId + '/' + srcOne);
      })
      .then(embedder.test.succeed, embedder.test.fail);
};

// Call load with the same extension Id and src.
function testLoadAPISameIdAndSrc(extensionId) {
  var extensionview = document.querySelector('extensionview');
  extensionview.load(extensionScheme + extensionId + '/' + srcOne)
      .then(function() {
        return extensionview.load(extensionScheme + extensionId + '/' + srcOne);
      })
      .then(function() {
        checkExtensionAttribute(extensionview, extensionId);
        checkSrcAttribute(
            extensionview, extensionScheme + extensionId + '/' + srcOne);
      })
      .then(embedder.test.succeed, embedder.test.fail);
};

// Call load with the same extension Id and different src.
function testLoadAPISameIdDifferentSrc(extensionId) {
  var extensionview = document.querySelector('extensionview');
  extensionview.load(extensionScheme + extensionId + '/' + srcOne)
      .then(function() {
        return extensionview.load(extensionScheme + extensionId + '/' + srcTwo);
      })
      .then(function() {
        checkExtensionAttribute(extensionview, extensionId);
        checkSrcAttribute(
            extensionview, extensionScheme + extensionId + '/' + srcTwo);
      })
      .then(embedder.test.succeed, embedder.test.fail);
};

// Call load with a new extension Id and src.
function testLoadAPILoadOtherExtension(extensionIdOne, extensionIdTwo) {
  var extensionview = document.querySelector('extensionview');
  extensionview.load(extensionScheme + extensionIdOne + '/' + srcOne)
      .then(function() {
        return extensionview.load(
            extensionScheme + extensionIdTwo + '/' + srcTwo);
      })
      .then(function() {
        checkExtensionAttribute(extensionview, extensionIdTwo);
        checkSrcAttribute(
            extensionview, extensionScheme + extensionIdTwo + '/' + srcTwo);
        // Try another load of the first extension again to make sure the
        // previous load managed to complete without stalling the action queue.
        return extensionview.load(
            extensionScheme + extensionIdOne + '/' + srcOne);
      })
      .then(function() {
        checkExtensionAttribute(extensionview, extensionIdOne);
        checkSrcAttribute(
            extensionview, extensionScheme + extensionIdOne + '/' + srcOne);
      })
      .then(embedder.test.succeed, embedder.test.fail);
};

// Call load with an invalid extension.
function testLoadAPIInvalidExtension() {
  var invalidExtensionId = 'fakeExtension';
  var extensionview = document.querySelector('extensionview');
  extensionview.load(extensionScheme + invalidExtensionId + '/' + srcOne)
      .then(embedder.test.fail, embedder.test.succeed);
};

// Call load with a valid extension Id and src after an invalid call.
function testLoadAPIAfterInvalidCall(extensionId) {
  var invalidExtensionId = 'fakeExtension';
  var extensionview = document.querySelector('extensionview');
  extensionview.load(extensionScheme + invalidExtensionId + '/' + srcOne)
      .then(
          embedder.test.fail,
          function() {
            return extensionview.load(
                extensionScheme + extensionId + '/' + srcTwo);
          })
      .then(embedder.test.succeed, embedder.test.fail);
};

// Call load with a null extension.
function testLoadAPINullExtension() {
  var extensionview = document.querySelector('extensionview');
  extensionview.load(null).then(embedder.test.fail, embedder.test.succeed);
};

function testQueuedLoadAPIFunction(extensionId) {
  var extensionview = document.querySelector('extensionview');

  var loadCallCount = 0;
  var load_promises = [];

  // Call load a first time with a specified extension ID and src.
  load_promises.push(
      extensionview.load(extensionScheme + extensionId + '/' + srcOne)
          .then(function() {
            loadCallCount++;
            embedder.test.assertEq(1, loadCallCount);
          }));

  // Call load a second time with the same extension Id and src.
  load_promises.push(
      extensionview.load(extensionScheme + extensionId + '/' + srcOne)
          .then(function() {
            loadCallCount++;
            embedder.test.assertEq(2, loadCallCount);
          }));

  // Call load a third time with the same extension Id and different src.
  load_promises.push(
      extensionview.load(extensionScheme + extensionId + '/' + srcTwo)
          .then(function() {
            loadCallCount++;
            embedder.test.assertEq(3, loadCallCount);
          }));

  Promise.all(load_promises)
      .then(function() {
        // Ensure we have the expected attributes for the most recent load.
        checkExtensionAttribute(extensionview, extensionId);
        checkSrcAttribute(
            extensionview, extensionScheme + extensionId + '/' + srcTwo);
      })
      .then(embedder.test.succeed, embedder.test.fail);
};

function testQueuedLoadAPILoadOtherExtension(extensionIdOne, extensionIdTwo) {
  var extensionview = document.querySelector('extensionview');

  var loadCallCount = 0;
  var load_promises = [];

  load_promises.push(
      extensionview.load(extensionScheme + extensionIdOne + '/' + srcOne)
          .then(function() {
            loadCallCount++;
            embedder.test.assertEq(1, loadCallCount);
          }));

  // Enqueue a load to another extension.
  load_promises.push(
      extensionview.load(extensionScheme + extensionIdTwo + '/' + srcTwo)
          .then(function() {
            loadCallCount++;
            embedder.test.assertEq(2, loadCallCount);
          }));

  // Enqueue a load to back to the original extension.
  load_promises.push(
      extensionview.load(extensionScheme + extensionIdOne + '/' + srcOne)
          .then(function() {
            loadCallCount++;
            embedder.test.assertEq(3, loadCallCount);
          }));

  Promise.all(load_promises)
      .then(function() {
        // Ensure we have the expected attributes for the most recent load.
        checkExtensionAttribute(extensionview, extensionIdOne);
        checkSrcAttribute(
            extensionview, extensionScheme + extensionIdOne + '/' + srcOne);
      })
      .then(embedder.test.succeed, embedder.test.fail);
};

embedder.test.testList = {
  'testLoadAPIFunction': testLoadAPIFunction,
  'testLoadAPISameIdAndSrc': testLoadAPISameIdAndSrc,
  'testLoadAPISameIdDifferentSrc': testLoadAPISameIdDifferentSrc,
  'testLoadAPILoadOtherExtension': testLoadAPILoadOtherExtension,
  'testLoadAPIInvalidExtension': testLoadAPIInvalidExtension,
  'testLoadAPIAfterInvalidCall': testLoadAPIAfterInvalidCall,
  'testLoadAPINullExtension': testLoadAPINullExtension,
  'testQueuedLoadAPIFunction': testQueuedLoadAPIFunction,
  'testQueuedLoadAPILoadOtherExtension': testQueuedLoadAPILoadOtherExtension,
};

onload = function() {
  chrome.test.getConfig(function(config) {
    embedder.setUp_(config);
    chrome.test.sendMessage('Launched');
  });
};
