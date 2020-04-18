// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO(xunjieli): When URLSearchParams is stable and implemented, switch this
// (and a lot of other test code) to it. https://crbug.com/303152
var url = decodeURIComponent(/url=([^&]*)/.exec(location.search)[1]);
var filter = {urls: ['http://www.example.com/*'], types: ['xmlhttprequest']};

chrome.webRequest.onCompleted.addListener(function(details) {
  chrome.test.notifyFail('Unexpected.');
}, filter);

chrome.webRequest.onErrorOccurred.addListener(function(details) {
  chrome.test.assertEq('net::ERR_TEMPORARILY_THROTTLED', details.error);
  chrome.test.notifyPass();
}, filter);

chrome.runtime.sendMessage({type: 'xhr', method: 'GET', url: url});
