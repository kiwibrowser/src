// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

window.pacRequestCount = 0;
window.title2RequestCount = 0;

chrome.webRequest.onBeforeRequest.addListener(function(details) {
  ++window.pacRequestCount;
}, {urls: ['*://*/self.pac']});

chrome.webRequest.onBeforeRequest.addListener(function(details) {
  ++window.title2RequestCount;
}, {urls: ['*://*/title2.html']});

chrome.test.sendMessage('ready');
