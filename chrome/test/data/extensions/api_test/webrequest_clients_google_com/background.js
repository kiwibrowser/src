// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

window.clientsGoogleWebRequestCount = 0;
window.yahooWebRequestCount = 0;

chrome.webRequest.onBeforeRequest.addListener(function(details) {
  if (details.url.includes('http://clients1.google.com/'))
    ++window.clientsGoogleWebRequestCount;
  if (details.url.includes('http://yahoo.com'))
    ++window.yahooWebRequestCount;
}, {urls: ['<all_urls>']});

chrome.test.sendMessage('ready');
