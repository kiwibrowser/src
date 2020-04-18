// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var tests = [
  function getPrinters() {
    chrome.cloudPrintPrivate.getPrinters(
        function(result) {
            chrome.test.assertLastError("Cannot access in incognito mode");
            chrome.test.succeed();
        });
  }
];

chrome.test.runTests(tests);
