// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This test verifies that the <webview> API is undefined if the webview
// permission is not specified in the manifest file.
function testAPIMethodExistence() {
  var apiMethodsToCheck = [
    'back',
    'canGoBack',
    'canGoForward',
    'forward',
    'getProcessId',
    'go',
    'reload',
    'stop',
    'terminate'
  ];
  var webview = document.createElement('webview');
  for (var i = 0; i < apiMethodsToCheck.length; ++i) {
    chrome.test.assertEq('undefined',
                         typeof webview[apiMethodsToCheck[i]]);
  }

  // Check contentWindow.
  chrome.test.assertEq('undefined', typeof webview.contentWindow);
  chrome.test.succeed();
}

chrome.test.runTests([
  testAPIMethodExistence
]);
