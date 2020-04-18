// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var allTests = [
  function testFocusInIframes() {
    chrome.test.getConfig(function(config) {
      var url = 'http://a.com:' + config.testServer.port + '/iframe_outer.html';
      chrome.tabs.create({url: url});

      chrome.automation.getDesktop(function(rootNode) {
        // Succeed when the button inside the iframe gets focus.
        rootNode.addEventListener('focus', function(event) {
          if (event.target.name == 'Inner')
            chrome.test.succeed();
        });

        // Wait for the inner frame to load, then find the button inside it
        // and focus it.
        rootNode.addEventListener('loadComplete', function(event) {
          if (event.target.url.indexOf('iframe_inner.html') >= 0) {
            chrome.automation.getFocus(function(focus) {
              // Assert that the outer frame has focus.
              assertTrue(focus.url.indexOf('iframe_outer') >= 0);

              // Find the inner button and focus it.
              var innerButton = focus.find({ attributes: { name: 'Inner' } });
              innerButton.focus();
            });
          }
        });
      });
    });
  },
];

chrome.test.runTests(allTests);
