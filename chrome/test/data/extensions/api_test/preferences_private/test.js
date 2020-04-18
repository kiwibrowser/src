// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.test.runTests([
  function testOnChangedExists() {
    // Verify that the
    // chrome.preferencesPrivate.easyUnlockProximityRequired.onChange event
    // exists. It doesn't need to do anything other than support adding/removing
    // listeners for backwards compatibility.
    chrome.test.assertTrue(!!chrome.preferencesPrivate);
    chrome.test.assertTrue(
        !!chrome.preferencesPrivate.easyUnlockProximityRequired);
    var onChange =
        chrome.preferencesPrivate.easyUnlockProximityRequired.onChange;
    chrome.test.assertTrue(!!onChange);
    chrome.test.assertEq('function', typeof onChange.addListener);
    chrome.test.assertEq('function', typeof onChange.removeListener);
    chrome.test.succeed();
  },
]);
