// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var authAttempted = false;

chrome.screenlockPrivate.onChanged.addListener(function(locked) {
  if (locked && !authAttempted) {
    chrome.test.sendMessage('attemptClickAuth');
  } else if (!locked && authAttempted) {
    chrome.test.succeed();
  } else {
    var action = locked ? "lock" : "unlock";
    chrome.test.fail('Unexpected ' + action);
  }
});

chrome.screenlockPrivate.onAuthAttempted.addListener(function(authType, input) {
  chrome.test.assertEq('userClick', authType);
  authAttempted = true;
  chrome.screenlockPrivate.acceptAuthAttempt(true);
});

chrome.test.runTests([
    function testAuthType() {
      chrome.screenlockPrivate.setLocked(true);
    }
]);
