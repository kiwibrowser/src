// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var allTests = [
  function testCloseTab() {
    getUrlFromConfig('index.html', function(url) {
      chrome.tabs.create({'url': url}, function(tab) {
        chrome.automation.getTree(function(rootNode) {
          var button = rootNode.find({role: 'button'});
          assertEq(rootNode, button.root);

          rootNode.addEventListener('destroyed', function() {
            // Poll until the root node doesn't have a role anymore
            // indicating that it really did get cleaned up.
            function checkSuccess() {
              if (rootNode.role === undefined && rootNode.root === null) {
                assertEq(null, button.root);
                chrome.test.succeed();
              }
              else
                window.setTimeout(checkSuccess, 10);
            }
            checkSuccess();
          });
          chrome.tabs.remove(tab.id);
        });
      });
    });
  }
]
chrome.test.runTests(allTests);
