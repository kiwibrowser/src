// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var tests = [
  function successfulSetupConnector() {
    var userEmail = 'foo@gmail.com';
    var robotEmail = 'foorobot@googleusercontent.com';
    var credentials = '1/23546efa54';
    chrome.cloudPrintPrivate.setupConnector(
        userEmail, robotEmail, credentials, {
          "connectNewPrinters": true,
          "printers": [
            {
              "name" : "printer1",
              "connect" : false
            }, {
              "name" : "printer2",
              "connect" : true
            }
          ]
        });
    chrome.test.succeed();
  },
  function getHostName() {
    chrome.cloudPrintPrivate.getHostName(
        chrome.test.callbackPass(function(result) {
            chrome.test.assertNoLastError();
            chrome.test.assertEq("TestHostName", result);
        }));
  },
  function getPrinters() {
    chrome.cloudPrintPrivate.getPrinters(
        chrome.test.callbackPass(function(result) {
            chrome.test.assertNoLastError();
            chrome.test.assertEq(result, ['printer1', 'printer2']);
        }));
  },
  function getClientId() {
    chrome.cloudPrintPrivate.getClientId(
        chrome.test.callbackPass(function(result) {
            chrome.test.assertNoLastError();
            chrome.test.assertEq("TestAPIClient", result);
        }));
  }
];

chrome.test.runTests(tests);
