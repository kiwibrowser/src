// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var allTests = [
  function testSendModifiedKeyEvent() {
    let tabCount = 0;
    chrome.tabs.onCreated.addListener(function(tab) {
      tabCount++;
      if (tabCount == 2)
        chrome.test.succeed();
    });

    chrome.accessibilityPrivate.sendSyntheticKeyEvent({
      type: 'keydown',
      keyCode: 84 /* T */,
      modifiers: {
        ctrl: true
      }
    });

    chrome.accessibilityPrivate.sendSyntheticKeyEvent({
      type: 'keydown',
      keyCode: 84 /* T */,
      modifiers: {
        ctrl: true,
        alt: false,
        shift: false,
        search: false
      }
    });
  }
];

chrome.test.runTests(allTests);
