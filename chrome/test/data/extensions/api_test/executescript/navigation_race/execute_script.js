// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function executeCodeInTab(tabId, callback) {
  chrome.tabs.executeScript(
      tabId,
      {code: "document.title = 'hi, I\\'m on ' + location;"},
      callback);
}
