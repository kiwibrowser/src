// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

var assertEq = chrome.test.assertEq;
var assertTrue = chrome.test.assertTrue;
var assertThrows = chrome.test.assertThrows;
var fail = chrome.test.fail;
var succeed = chrome.test.succeed;
var callbackPass = chrome.test.callbackPass;
var callbackFail = chrome.test.callbackFail;

var testCases = {
  neitherHexSsidNorSsid:
      {input: [{Type: 'WiFi'}], error: 'Malformed filter description.'},
  ssid: {input: [{Type: 'WiFi', SSID: 'SSID1'}]},
  hexSsid: {input: [{Type: 'WiFi', HexSSID: '5353494431'}]},
  invalidHexSsid1: {
    input: [{Type: 'WiFi', HexSSID: '5'}],
    error:
        'Malformed filter description. Failed to register network with SSID ' +
        '(hex): 5'
  },
  invalidHexSsid2: {
    input: [{Type: 'WiFi', HexSSID: 'ABCDEFGH'}],
    error:
        'Malformed filter description. Failed to register network with SSID ' +
        '(hex): ABCDEFGH'
  }
};

var runTests = function() {
  for (var key in testCases) {
    var testCase = testCases[key];
    if (testCase.hasOwnProperty('error'))
      var callback = callbackFail(testCase.error);
    else
      var callback = callbackPass();
    chrome.networking.config.setNetworkFilter(testCase.input, callback);
  }
};

runTests();
