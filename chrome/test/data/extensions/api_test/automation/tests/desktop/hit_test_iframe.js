// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var allTests = [
  function testHitTestInIframe() {
    chrome.test.getConfig(function(config) {
      var url = 'http://a.com:' + config.testServer.port + '/iframe_outer.html';
      chrome.tabs.create({url: url});

      chrome.automation.getDesktop(function(rootNode) {
        // Succeed when the button inside the iframe gets a HOVER event.
        rootNode.addEventListener(EventType.HOVER, function(event) {
          if (event.target.name == 'Inner')
            chrome.test.succeed();
        });

        // Wait for the inner frame to load, then find the button inside it
        // and do a hit test on it.
        rootNode.addEventListener(EventType.LOAD_COMPLETE, function(event) {
          if (event.target.url.indexOf('iframe_inner.html') >= 0) {
            // Find the inner button.
            var innerButton = event.target.find(
                { attributes: { name: 'Inner' } });
            var bounds = innerButton.location;
            var x = Math.floor(bounds.left + bounds.width / 2);
            var y = Math.floor(bounds.top + bounds.height / 2);
            rootNode.hitTest(x, y, EventType.HOVER);
          }
        });
      });
    });
  },
];

chrome.test.runTests(allTests);
