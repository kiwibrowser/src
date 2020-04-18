// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Register a proxy setting, and inform the browser.
var config = {
  mode: "auto_detect"
};

chrome.proxy.settings.set(
    {'value': { mode: 'auto_detect' } },
    function() { chrome.test.sendMessage('registered');
});
