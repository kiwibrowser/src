// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

var assertEq = chrome.test.assertEq;
var assertTrue = chrome.test.assertTrue;
var fail = chrome.test.fail;
var succeed = chrome.test.succeed;
var callbackPass = chrome.test.callbackPass;

chrome.networking.config.onCaptivePortalDetected.addListener(
    function(networkInfo) {
      function onDetectionTest() {
        assertTrue(!chrome.runtime.lastError);
        var expectedInfo = {
          Type: 'WiFi',
          GUID: 'wifi1_guid',
          SSID: 'wifi1',
          HexSSID: '7769666931',
          BSSID: '01:02:ab:7f:90:00'
        };
        assertEq(expectedInfo, networkInfo);
        chrome.networking.config.finishAuthentication(
            networkInfo.GUID, 'succeeded', function() {
              if (chrome.runtime.lastError) {
                fail(chrome.runtime.lastError.message);
              }
            });
        // This hands control back to C++.
        succeed();
      }

      chrome.test.runTests([onDetectionTest]);
    });

function initialSetupTest() {
  // |callbackPass()| will hand control back to the test in C++, which will
  // trigger an |onCaptivePortalDetected| event that is handled by the listener
  // above.
  chrome.networking.config.setNetworkFilter([{Type: 'WiFi', SSID: 'wifi1'}],
                                            callbackPass());
}

chrome.test.runTests([initialSetupTest]);
