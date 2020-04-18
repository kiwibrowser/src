// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var callback = chrome.test.callback;

// The names of the extensions to operate on, from their manifest.json files.
var ENABLED_NAME = 'enabled_extension';
var DISABLED_NAME = 'disabled_extension';
var UNINSTALL_NAME = 'enabled_extension';

// Given a list of extension |items|, finds the one with the given |name|.
function findByName(items, name) {
  chrome.test.assertEq(8, items.length);
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

var tests = [
  // Tests disabling an extension, expecting it to succeed.
  function allowedDisable() {
    chrome.management.getAll(callback(function(items) {
      var item = findByName(items, ENABLED_NAME);
      console.log(item);
      chrome.test.assertEq(true, item.mayDisable);
      chrome.test.assertEq(true, item.enabled);

      var id = item.id;
      chrome.management.setEnabled(id, false, callback(function() {
        chrome.management.get(id, callback(function(same_extension) {
          chrome.test.assertEq(false, same_extension.enabled);
        }));
      }));
    }));
  },

  // Tests enabling an extension, expecting it to succeed.
  function allowedEnable() {
    chrome.management.getAll(callback(function(items) {
      var item = findByName(items, DISABLED_NAME);
      chrome.test.assertEq(true, item.mayDisable);
      chrome.test.assertEq(false, item.enabled);

      var id = item.id;
      chrome.management.setEnabled(id, true, callback(function() {
        chrome.management.get(id, callback(function(same_extension) {
          chrome.test.assertEq(true, same_extension.enabled);
        }));
      }));
    }));
  },

  // Tests uninstalling an extension, expecting it to succeed.
  function allowedUninstall() {
    chrome.management.getAll(callback(function(items) {
      var item = findByName(items, UNINSTALL_NAME);
      chrome.test.assertEq(true, item.mayDisable);

      var id = item.id;
      chrome.test.runWithUserGesture(function() {
        chrome.management.uninstall(id, callback(function() {
          chrome.test.assertNoLastError();
          // The calling api test will verify that the item was uninstalled.
        }));
      });
    }));
  }
];

chrome.test.runTests(tests);
