// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var callbackPass = chrome.test.callbackPass;
var callbackFail = chrome.test.callbackFail;
var assertTrue = chrome.test.assertTrue;
var assertEq = chrome.test.assertEq;

// Test properties for the verification API.
var verificationProperties = {
  certificate: 'certificate',
  intermediateCertificates: ['ica1', 'ica2', 'ica3'],
  publicKey: 'cHVibGljX2tleQ==',  // Base64('public_key')
  nonce: 'nonce',
  signedData: 'c2lnbmVkX2RhdGE=',  // Base64('signed_data')
  deviceSerial: 'device_serial',
  deviceSsid: 'Device 0123',
  deviceBssid: '00:01:02:03:04:05'
};

chrome.test.getConfig(function(config) {
  var args = JSON.parse(config.customArg);

  chrome.test.runTests([
    function verifyDestination() {
      chrome.networking.castPrivate.verifyDestination(
          verificationProperties,
          callbackPass(function(isValid) {
            assertTrue(isValid);
          }));
    },
    function verifyAndEncryptCredentials() {
      var networkGuid = 'wifi_guid';
      chrome.networking.castPrivate.verifyAndEncryptCredentials(
          verificationProperties,
          networkGuid,
          callbackPass(function(result) {
            assertEq('encrypted_credentials', result);
          }));
    },
    function verifyAndEncryptData() {
      chrome.networking.castPrivate.verifyAndEncryptData(
          verificationProperties,
          'data',
          callbackPass(function(result) {
            assertEq('encrypted_data', result);
          }));
    },
    function setWifiTDLSEnabledState() {
      if (args.tdlsSupported) {
        chrome.networking.castPrivate.setWifiTDLSEnabledState(
            'aa:bb:cc:dd:ee:ff', true, callbackPass(function(result) {
              assertEq('CONNECTED', result);
            }));
      } else {
        chrome.networking.castPrivate.setWifiTDLSEnabledState(
            'aa:bb:cc:dd:ee:ff', true, callbackFail('Not supported'));
      }
    },
    function getWifiTDLSStatus() {
      if (args.tdlsSupported) {
        chrome.networking.castPrivate.getWifiTDLSStatus(
            'aa:bb:cc:dd:ee:ff', callbackPass(function(result) {
              assertEq('CONNECTED', result);
            }));
      } else {
        chrome.networking.castPrivate.getWifiTDLSStatus(
            'aa:bb:cc:dd:ee:ff', callbackFail('Not supported'));
      }
    },
  ]);
});
