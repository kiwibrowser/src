// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var callbackFail = chrome.test.callbackFail;
var callbackPass = chrome.test.callbackPass;
var expectedError =
    "Invalid value for origin pattern file:///Invalid scheme.: *";

function test() {
  chrome.permissions.request({"origins": ["file:///*"]},
                             callbackFail(expectedError, function(granted) {
    chrome.test.assertFalse(!!granted);
    chrome.permissions.getAll(callbackPass(function(permissions) {
      chrome.test.assertEq([], permissions.origins);
      chrome.test.succeed();
    }));
  }));
}

chrome.test.runTests([test]);
