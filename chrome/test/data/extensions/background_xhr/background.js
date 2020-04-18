// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.runtime.onMessage.addListener(function(message, sender, sendResponse) {
  if (message.type == "xhr") {
    var xhr = new XMLHttpRequest();
    xhr.open(message.method, message.url);
    xhr.send();
  } else {
    console.error("Unknown message: " + JSON.stringify(message));
  }
});
