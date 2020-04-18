// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.test.sendMessage('ready', function() {
  chrome.browserAction.openPopup(function(popup) {
    chrome.test.assertFalse(!!popup);
    chrome.test.notifyPass();
  });
});
