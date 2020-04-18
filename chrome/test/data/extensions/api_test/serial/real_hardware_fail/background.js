// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var testGetDevices = function() {
  var onGetDevices = function(devices) {
    chrome.test.assertTrue(devices.length == 0);
    chrome.test.succeed();
  }

  chrome.serial.getDevices(onGetDevices);
};

var tests = [testGetDevices];
chrome.test.runTests(tests);
