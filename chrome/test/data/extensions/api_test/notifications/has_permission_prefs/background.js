// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.test.runTests([
  function hasPermission() {
    chrome.test.assertEq("granted",  // permission not allowed
                         Notification.permission);
    chrome.test.succeed();
  },
  function showTextNotification() {
    var notification = new Notification("Foo", {
      body: "This is a text notification."
    });
    notification.onerror = function() {
      chrome.test.fail("Failed to show notification.");
    };
    notification.onshow = function() {
      chrome.test.succeed();
    };
  }
]);
