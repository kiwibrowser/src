// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.test.runTests([
  function getPolicy() {
    chrome.storage.managed.get(
        'string-policy',
        chrome.test.callbackPass(function(results) {
          chrome.test.assertEq({
            'string-policy': 'value'
          }, results);
        }));
  },

  function getListOfPolicies() {
    chrome.storage.managed.get(
        ['string-policy', 'int-policy', 'no-such-thing'],
        chrome.test.callbackPass(function(results) {
          chrome.test.assertEq({
            'string-policy': 'value',
            'int-policy': -123,
          }, results);
        }));
  },

  function getAllPolicies() {
    chrome.storage.managed.get(
        chrome.test.callbackPass(function(results) {
          chrome.test.assertEq({
            'string-policy': 'value',
            'int-policy': -123,
            'double-policy': 456e7,
            'boolean-policy': true,
            'list-policy': [ 'one', 'two', 'three' ],
            'dict-policy': {
              'list': [ { 'one': 1, 'two': 2 }, { 'three': 3} ]
            }
          }, results);
        }));
  },

  function getBytesInUse() {
    chrome.storage.managed.getBytesInUse(
        chrome.test.callbackPass(function(bytes) {
          chrome.test.assertEq(0, bytes);
        }));
  },

  function writingFails() {
    var kReadOnlyError = 'This is a read-only store.';
    chrome.storage.managed.clear(chrome.test.callbackFail(kReadOnlyError));
    chrome.storage.managed.remove(
        'string-policy',
        chrome.test.callbackFail(kReadOnlyError));
    chrome.storage.managed.set({
      'key': 'value'
    }, chrome.test.callbackFail(kReadOnlyError));
  }
]);
