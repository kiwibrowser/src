// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var allTests = [
  function testGetDesktop() {
    chrome.automation.getDesktop(function(rootNode) {
      assertEq(RoleType.DESKTOP, rootNode.role);
      assertEq(undefined, rootNode.firstChild);
      chrome.test.succeed();
    });
  },

  function testGetDesktopTwice() {
    var desktop = null;
    chrome.automation.getDesktop(function(rootNode) {
      desktop = rootNode;
    });
    chrome.automation.getDesktop(function(rootNode) {
      assertEq(rootNode, desktop);
      chrome.test.succeed();
    });
  },

  function testGetDesktopNested() {
    var desktop = null;
    chrome.automation.getDesktop(function(rootNode) {
      desktop = rootNode;
      chrome.automation.getDesktop(function(rootNode2) {
        assertEq(rootNode2, desktop);
        chrome.test.succeed();
      });
    });
  },

  function testAutomationNodeToString() {
    chrome.automation.getDesktop(function(rootNode) {
      assertEq(RoleType.DESKTOP, rootNode.role);
      var prefix = 'tree id=0';
      assertEq(prefix, rootNode.toString().substring(0, prefix.length));
      chrome.test.succeed();
    });
  }
];

chrome.test.runTests(allTests);
