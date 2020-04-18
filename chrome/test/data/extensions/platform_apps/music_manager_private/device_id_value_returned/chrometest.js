// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function runTests() {
  chrome.test.runTests([
    function test() {
      chrome.musicManagerPrivate.getDeviceId(function(id) {
          console.log('Device ID=' + id);
          if (id) {
            // This block is not entered in VMs (e.g. the bots) which return
            // dummy MACs.
            chrome.test.assertEq('string', typeof id);
            chrome.test.assertTrue(id.length >= 8);
          }
          chrome.test.succeed();
      });
    }
  ]);
}

window.onload = function() {
  chrome.test.getConfig(function(config) {
    console.log('customArg=' + config.customArg);
    runTests();
  });
}
