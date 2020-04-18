// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var assertEq = chrome.test.assertEq;
var assertFalse = chrome.test.assertFalse;
var assertTrue = chrome.test.assertTrue;
var callbackFail = chrome.test.callbackFail;

var urlA = '';
var urlB = '';

var allTestsADomain = [
  function testSuccess() {
    chrome.automation.getTree(function(tree) {
      assertFalse(tree === undefined);
      chrome.test.succeed();
    });
  }
];

var allTestsBDomain = [
  function testError() {
    var expectedError = 'Cannot request automation tree on url "' + urlB +
        '". Extension manifest must request permission to access this host.';
    chrome.automation.getTree(callbackFail(expectedError, function(tree) {
      assertEq(undefined, tree);
      chrome.test.succeed();
    }));
  }
];

chrome.test.getConfig(function(config) {
  assertTrue('testServer' in config, 'Expected testServer in config');
  urlA = 'http://a.com:PORT/index.html'
      .replace(/PORT/, config.testServer.port);

  chrome.tabs.create({ 'url': urlA }, function() {
    chrome.test.runTests(allTestsADomain);

    urlB = 'http://b.com:PORT/index.html'
        .replace(/PORT/, config.testServer.port);

    chrome.tabs.create({ 'url': urlB }, function() {
      chrome.test.runTests(allTestsBDomain);
    });
  });
});

