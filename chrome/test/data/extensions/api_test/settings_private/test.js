// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This just tests the interface. It does not test for specific results, only
// that callbacks are correctly invoked, expected parameters are correct,
// and failures are detected.

var kTestPrefName = 'autofill.enabled';
var kTestPrefValue = true;

// This corresponds to policy key: kHomepageIsNewTabPage used in
// settings_private_apitest.cc.
var kTestEnforcedPrefName = 'homepage_is_newtabpage';

var kTestPageId = 'pageId';

function callbackResult(result) {
  if (chrome.runtime.lastError)
    chrome.test.fail(chrome.runtime.lastError.message);
  else if (result == false)
    chrome.test.fail('Failed: ' + result);
}

var availableTests = [
  function setPref() {
    chrome.settingsPrivate.setPref(
        kTestPrefName,
        kTestPrefValue,
        kTestPageId,
        function(success) {
          callbackResult(success);
          chrome.test.succeed();
        });
  },
  function setPref_CrOSSetting() {
    chrome.settingsPrivate.setPref(
        'cros.accounts.allowBWSI',
        false,
        kTestPageId,
        function(success) {
          callbackResult(success);
          chrome.test.succeed();
        });
  },
  function getPref() {
    chrome.settingsPrivate.getPref(
        kTestPrefName,
        function(value) {
          chrome.test.assertTrue(value !== null);
          callbackResult(true);
          chrome.test.succeed();
        });
  },
  function getEnforcedPref() {
    chrome.settingsPrivate.getPref(kTestEnforcedPrefName, function(value) {
      chrome.test.assertEq('object', typeof value);
      callbackResult(true);
      chrome.test.assertEq(
          chrome.settingsPrivate.ControlledBy.USER_POLICY, value.controlledBy);
      chrome.test.assertEq(
          chrome.settingsPrivate.Enforcement.ENFORCED, value.enforcement);
      chrome.test.succeed();
    });
  },
  function getRecommendedPref() {
    chrome.settingsPrivate.getPref(kTestEnforcedPrefName, function(value) {
      chrome.test.assertEq('object', typeof value);
      callbackResult(true);
      chrome.test.assertEq(true, value.value);
      chrome.test.assertEq(
          chrome.settingsPrivate.ControlledBy.USER_POLICY, value.controlledBy);
      chrome.test.assertEq(
          chrome.settingsPrivate.Enforcement.RECOMMENDED, value.enforcement);
      // Set the value to false, policy properties should still be set.
      chrome.settingsPrivate.setPref(
          kTestEnforcedPrefName, false, kTestPageId, function(success) {
            callbackResult(success);
            chrome.settingsPrivate.getPref(
                kTestEnforcedPrefName, function(value) {
                  chrome.test.assertEq('object', typeof value);
                  callbackResult(true);
                  chrome.test.assertEq(false, value.value);
                  chrome.test.assertEq(
                      chrome.settingsPrivate.ControlledBy.USER_POLICY,
                      value.controlledBy);
                  chrome.test.assertEq(
                      chrome.settingsPrivate.Enforcement.RECOMMENDED,
                      value.enforcement);
                  chrome.test.succeed();
                });
          });
    });
  },
  function getPref_CrOSSetting() {
    chrome.settingsPrivate.getPref(
        'cros.accounts.allowBWSI',
        function(value) {
          chrome.test.assertTrue(value !== null);
          callbackResult(true);
          chrome.test.succeed();
        });
  },
  function getAllPrefs() {
    chrome.settingsPrivate.getAllPrefs(
        function(prefs) {
          chrome.test.assertTrue(prefs.length > 0);
          callbackResult(true);
          chrome.test.succeed();
        });
  },
  function onPrefsChanged() {
    chrome.settingsPrivate.onPrefsChanged.addListener(function(prefs) {
      chrome.test.assertTrue(prefs.length > 0);
      chrome.test.assertEq(kTestPrefName, prefs[0].key);
      chrome.test.assertEq(kTestPrefValue, prefs[0].value);
      callbackResult(true);
      chrome.test.succeed();
    });

    chrome.settingsPrivate.setPref(
        kTestPrefName,
        kTestPrefValue,
        kTestPageId,
        function() {});
  },
  function onPrefsChanged_CrOSSetting() {
    chrome.settingsPrivate.onPrefsChanged.addListener(function(prefs) {
      chrome.test.assertTrue(prefs.length > 0);
      chrome.test.assertEq('cros.accounts.allowBWSI', prefs[0].key);
      chrome.test.assertEq(false, prefs[0].value);
      callbackResult(true);
      chrome.test.succeed();
    });

    chrome.settingsPrivate.setPref(
        'cros.accounts.allowBWSI',
        false,
        kTestPageId,
        function() {});
  },
];

var testToRun = window.location.search.substring(1);
chrome.test.runTests(availableTests.filter(function(op) {
  return op.name == testToRun;
}));
