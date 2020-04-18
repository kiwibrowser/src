// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Test that we can change various properties of the browser action.
// The C++ verifies.
chrome.browserAction.setTitle({title: "Modified"});
chrome.browserAction.setIcon({path: "icon2.png"});
chrome.browserAction.setBadgeText({text: "badge"});
chrome.browserAction.setBadgeBackgroundColor({color: [255,255,255,255]});

chrome.test.notifyPass();
