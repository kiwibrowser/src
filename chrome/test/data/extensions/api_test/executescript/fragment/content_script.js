// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function notify() {
  chrome.extension.sendRequest("content_script");
}

if (document.readyState) {
  notify();
} else {
  document.onload = notify;
}
