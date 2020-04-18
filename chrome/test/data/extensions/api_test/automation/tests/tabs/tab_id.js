
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function createBackgroundTab(url, callback) {
  chrome.tabs.query({ active: true }, function(tabs) {
    chrome.test.assertEq(1, tabs.length);
    var originalActiveTab = tabs[0];
    createTab(url, function(tab) {
      chrome.tabs.update(originalActiveTab.id, { active: true }, function() {
        callback(tab);
      });
    })
  });
}

function assertCorrectTab(rootNode) {
  var title = rootNode.docTitle;
  chrome.test.assertEq('Automation Tests', title);
  chrome.test.succeed();
}

var allTests = [
  function testGetTabById() {
    getUrlFromConfig('index.html', function(url) {
      // Keep the NTP as the active tab so that we know we're requesting the
      // tab by ID rather than just getting the active tab still.
      createBackgroundTab(url, function(tab) {
        chrome.automation.getTree(tab.id, function(rootNode) {
          if (rootNode.docLoaded) {
            assertCorrectTab(rootNode);
            return;
          }

          rootNode.addEventListener('loadComplete', function() {
            assertCorrectTab(rootNode);
          });
        })
      });
    });
  }
];

chrome.test.runTests(allTests);
