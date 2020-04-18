// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// OnNaClLoad should be called and invoking a method
// in a NaCl module should return a correct value.

var pass = chrome.test.callbackPass;

chrome.test.runTests([
  function nacl() {
    // Nothing to do here,
    // we call the callback when we get the notification from NaCl
  }
]);

function OnNaClLoad() {
  try {
    plugin = document.getElementById('pluginobj');
    result = plugin.helloworld();
    if ('hello, world.' != result) {
      chrome.test.fail();
    }
  } catch(e) {
    chrome.test.fail();
  }
  chrome.test.succeed();
}

function OnNaClFail() {
  chrome.test.fail();
}
