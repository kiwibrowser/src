// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var callback = chrome.test.callback;

// The names of the extensions to operate on, from their manifest.json files.
var ENABLED_NAME = 'enabled_extension';
var DISABLED_NAME = 'disabled_extension';
var UNINSTALL_NAME = 'enabled_extension';

var EXPECTED_ERROR = 'Extension * cannot be modified by user.';

// Given a list of extension items, finds the one with the given name.
function findByName(items, name) {
  var item;
  for (var i = 0; i < items.length; i++) {
    item = items[i];
    if (item.name == name)
      break;
  }
  if (name != item.name)
    chrome.test.fail('Couldn\'t find installed extension ' + name);
  return item;
}

// Verifies that the extension with the given |id| has the expected |enabled|
// status.
function checkEnabled(id, enabled) {
  chrome.management.get(id, callback(function(result) {
    chrome.test.assertEq(enabled, result.enabled);
  }));
}

var tests = [
  // Tests disabling an extension, expecting it to fail.
  function prohibitedDisable() {
    chrome.management.getAll(callback(function(items) {
      var item = findByName(items, ENABLED_NAME);
      chrome.test.assertEq(false, item.mayDisable);
      chrome.test.assertEq(true, item.enabled);

      var id = item.id;
      var expectedError = EXPECTED_ERROR.replace('*', id);
      chrome.management.setEnabled(id, false,
          callback(function() { checkEnabled(id, true); }, expectedError));
    }));
  },

  // Tests enabling an extension, expecting it to fail.
  function prohibitedEnable() {
    chrome.management.getAll(callback(function(items) {
      var item = findByName(items, DISABLED_NAME);
      chrome.test.assertEq(false, item.mayDisable);
      chrome.test.assertEq(false, item.enabled);

      var id = item.id;
      var expectedError = EXPECTED_ERROR.replace('*', id);
      chrome.management.setEnabled(id, true,
          callback(function() { checkEnabled(id, false); }, expectedError));
    }));
  },

  // Tests uninstalling an extension, expecting it to fail.
  function prohibitedUninstall() {
    chrome.management.getAll(callback(function(items) {
      var item = findByName(items, UNINSTALL_NAME);
      chrome.test.assertEq(false, item.mayDisable);

      var id = item.id;
      var expectedError = EXPECTED_ERROR.replace('*', id);
      chrome.test.runWithUserGesture(function() {
        chrome.management.uninstall(id,
            callback(function() { checkEnabled(id, true); }, expectedError));
      });
    }));
  }
];

chrome.test.runTests(tests);
