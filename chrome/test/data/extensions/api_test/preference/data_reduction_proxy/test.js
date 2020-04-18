// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Content settings API test
// Run with browser_tests
// --gtest_filter=ExtensionPreferenceApiTest.DataReductionProxy

var dataReductionProxy = chrome.dataReductionProxy;
chrome.test.runTests([
  function getDrpPrefs() {
    dataReductionProxy.spdyProxyEnabled.get({}, chrome.test.callbackPass(
        function(result) {
          chrome.test.assertEq(
              {
                'value': false,
                'levelOfControl': 'controllable_by_this_extension'
              },
              result);
    }));
  },
  function clearDataSavings() {
    dataReductionProxy.dataUsageReportingEnabled.set({ 'value': true });
    dataReductionProxy.spdyProxyEnabled.set({ 'value': true });

    verifyDataUsage(20, chrome.test.callbackPass(function() {
      dataReductionProxy.clearDataSavings(function() {
        verifyDataUsage(1, null);
      });
    }));
  },
  function dataUsageReporting() {
    dataReductionProxy.dataUsageReportingEnabled.set({ 'value': true });

    verifyDataUsage(20, null);
  }
]);

// Data usage reporting takes some time to initialize before a call to
// |getDataUsage| is successful. If |getDataUsage| gives us an empty array,
// we retry after some delay. Test will report failure if the expected data
// usage is not returned after |numRetries| retries.
// We don't have a way to populate actual data usage in a browser test, so we
// only check the length of the returned data usage array.
// The |onVerifyDone| callback, if present, is executed after verifying data
// usage.
function verifyDataUsage(numRetries, onVerifyDone) {
  chrome.test.assertTrue(numRetries != 0);

  setTimeout(chrome.test.callbackPass(function() {
    dataReductionProxy.getDataUsage(chrome.test.callbackPass(
      function(data_usage) {
        chrome.test.assertTrue('data_usage_buckets' in data_usage);
        if (data_usage['data_usage_buckets'].length == 0) {
          verifyDataUsage(numRetries - 1, onVerifyDone);
        } else {
          chrome.test.assertEq(5760,
                               data_usage['data_usage_buckets'].length);
          if (onVerifyDone) {
            onVerifyDone();
          }
        }
    }));
  }), 1000);
};