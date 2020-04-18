// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var assertEq = chrome.test.assertEq;
var assertTrue = chrome.test.assertTrue;
var numReceivedRequests = 0;
var relativePath = 'extensions/api_test/executescript/permissions/';
var testFile = relativePath + 'empty.html';
var testFileFrames = relativePath + 'frames.html';
var onTabLoaded;

chrome.extension.onRequest.addListener(function(req) {
  numReceivedRequests++;
});

chrome.tabs.onUpdated.addListener(function(tabId, changeInfo, tab) {
  if (tab.status == 'complete' && onTabLoaded)
    onTabLoaded(tab);
});

chrome.test.getConfig(function(config) {

  function fixPort(url) {
    return url.replace(/PORT/, config.testServer.port);
  }

  chrome.test.runTests([
    // Test a race that used to occur here (see bug 30937).
    // Open a tab that we're not allowed to execute in (c.com), then
    // navigate it to a tab we *are* allowed to execute in (a.com),
    // then quickly run script in the tab before it navigates. It
    // should appear to work (no error -- it could have been a developer
    // mistake), but not actually do anything.
    function() {
      chrome.tabs.create({url: fixPort('http://c.com:PORT/') + testFile});
      onTabLoaded = function(tab) {
        onTabLoaded = null;
        numReceivedRequests = 0;
        chrome.tabs.update(
            tab.id, {url: fixPort('http://a.com:PORT/') + testFile});
        chrome.tabs.executeScript(tab.id, {file: 'script.js'});
        window.setTimeout(function() {
          assertEq(0, numReceivedRequests);
          chrome.test.succeed();
        }, 4000);
      };
    },

    // Inject into all frames. This should only affect frames we have
    // access to. This page has three subframes, one each from a.com,
    // b.com, and c.com. We have access to two of those, plus the root
    // frame, so we should get three responses.
    function() {
      chrome.tabs.create(
          {url: fixPort('http://a.com:PORT/') + testFileFrames});
      onTabLoaded = function(tab) {
        numReceivedRequests = 0;
        chrome.tabs.executeScript(tab.id,
                                  {file: 'script.js', allFrames: true});
        window.setTimeout(function() {
          chrome.test.assertEq(3, numReceivedRequests);
          chrome.test.succeed();
        }, 4000);
      };
    }
  ]);

});
