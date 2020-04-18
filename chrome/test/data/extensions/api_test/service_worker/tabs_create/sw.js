// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

self.onmessage = function(e) {
  if (e.data == 'createTab') {
    try {
      chrome.tabs.create({'url': 'http://www.google.com'}, function(tabs) {
        e.ports[0].postMessage('chrome.tabs.create callback');
      });
    } catch (e) {
      e.ports[0].postMessage('FAILURE');
    }
  } else {
    e.ports[0].postMessage('FAILURE');
  }
};
