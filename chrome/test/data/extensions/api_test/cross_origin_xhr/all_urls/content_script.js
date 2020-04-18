// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.extension.onRequest.addListener(
  function(url, sender, sendResponse) {
    var req = new XMLHttpRequest();
    req.open('GET', url, true);

    req.onload = function() {
      sendResponse({
        'event': 'load',
        'status': req.status,
        'text': req.responseText
      });
    };
    req.onerror = function() {
      sendResponse({
        'event': 'error',
        'status': req.status,
        'text': req.responseText
      });
    };

    req.send(null);
  });

chrome.extension.sendRequest('injected');

