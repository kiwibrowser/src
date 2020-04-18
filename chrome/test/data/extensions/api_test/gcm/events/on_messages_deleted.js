// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

onload = function() {
  chrome.test.runTests([
    function messagesDeleted() {
      chrome.test.listenOnce(chrome.gcm.onMessagesDeleted, function() {
        chrome.test.assertTrue(true);
      });
    }
  ]);
};
