// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.tabs.getSelected(null, function(tab) {
  chrome.browserAction.setPopup({
    popup: 'another_popup.html'
  });
  chrome.test.notifyPass();
});
