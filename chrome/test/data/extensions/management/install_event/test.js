// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.management.onInstalled.addListener(function(info) {
  if (info.name == "enabled_extension") {
    chrome.test.sendMessage("got_event");
  }
});

chrome.test.sendMessage("ready");
