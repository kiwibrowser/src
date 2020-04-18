// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.bluetoothSocket.create(
  function(socket) {
    if (!chrome.runtime.lastError) {
      chrome.test.fail("Expected an error");
    }
    chrome.test.assertEq("Permission denied", chrome.runtime.lastError.message);
    chrome.test.succeed();
  });
