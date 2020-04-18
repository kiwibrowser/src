// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var callbackPass = chrome.test.callbackPass;
var callbackFail = chrome.test.callbackFail;
var checkUnordererArrayEquality = function(expected, actual) {
  chrome.test.assertEq(expected.length, actual.length);

  var expectedSet = new Set(expected);
  for (var i = 0; i < actual.length; ++i) {
    chrome.test.assertTrue(
        expectedSet.has(actual[i]),
        JSON.stringify(actual[i]) + ' is not in the expected set');
  }
};

chrome.test.runTests([
  function addWhitelistedPages() {
    // Any duplicates in arguments will be filtered out.
    var toAdd = [
      'https://www.google.com/', 'https://www.google.com/',
      'https://www.yahoo.com/'
    ];
    chrome.declarativeNetRequest.addWhitelistedPages(
        toAdd, callbackPass(function() {}));
  },

  function verifyAddWhitelistedPages() {
    chrome.declarativeNetRequest.getWhitelistedPages(
        callbackPass(function(patterns) {
          checkUnordererArrayEquality(
              ['https://www.google.com/', 'https://www.yahoo.com/'], patterns);
        }));
  },

  function removeWhitelistedPages() {
    // It's ok for |toRemove| to specify a pattern which is not currently
    // whitelisted.
    var toRemove = ['https://www.google.com/', 'https://www.reddit.com/'];
    chrome.declarativeNetRequest.removeWhitelistedPages(
        toRemove, callbackPass(function() {}));
  },

  function verifyRemoveWhitelistedPages() {
    chrome.declarativeNetRequest.getWhitelistedPages(
        callbackPass(function(patterns) {
          checkUnordererArrayEquality(['https://www.yahoo.com/'], patterns);
        }));
  },

  function verifyErrorOnAddingInvalidMatchPattern() {
    // The second match pattern here is invalid. The current set of
    // whitelisted pages won't change since no addition is performed in case
    // any of the match patterns are invalid.
    var toAdd = ['https://www.reddit.com/', 'https://google*.com/'];
    var expectedError = 'Invalid url pattern \'https://google*.com/\'';
    chrome.declarativeNetRequest.addWhitelistedPages(
        toAdd, callbackFail(expectedError));
  },

  function verifyErrorOnRemovingInvalidMatchPattern() {
    // The second match pattern here is invalid since it has no path
    // component. current set of whitelisted pages won't change since no
    // removal is performed in case any of the match patterns are invalid.
    var toRemove = ['https://yahoo.com/', 'https://www.reddit.com'];
    var expectedError = 'Invalid url pattern \'https://www.reddit.com\'';
    chrome.declarativeNetRequest.removeWhitelistedPages(
        toRemove, callbackFail(expectedError));
  },

  function verifyExpectedPatternSetDidNotChange() {
    chrome.declarativeNetRequest.getWhitelistedPages(
        callbackPass(function(patterns) {
          checkUnordererArrayEquality(['https://www.yahoo.com/'], patterns);
        }));
  },

  function reachMaximumPatternLimit() {
    var toAdd = [];
    var numPatterns = 1;  // The extension already has one whitelisted pattern.
    while (numPatterns <
           chrome.declarativeNetRequest.MAX_NUMBER_OF_WHITELISTED_PAGES) {
      toAdd.push('https://' + numPatterns + '.com/');
      numPatterns++;
    }

    chrome.declarativeNetRequest.addWhitelistedPages(
        toAdd, callbackPass(function() {}));
  },

  function errorOnExceedingMaximumPatternLimit() {
    chrome.declarativeNetRequest.addWhitelistedPages(
        ['https://example.com/'],
        callbackFail(
            'The number of whitelisted page patterns can\'t exceed ' +
            chrome.declarativeNetRequest.MAX_NUMBER_OF_WHITELISTED_PAGES));
  },

  function addingDuplicatePatternSucceeds() {
    // Adding a duplicate pattern should still succeed since the final set of
    // whitelisted patterns is still at the limit.
    chrome.declarativeNetRequest.addWhitelistedPages(
        ['https://www.yahoo.com/'], callbackPass(function() {}));
  },

  function verifyPatterns() {
    chrome.declarativeNetRequest.getWhitelistedPages(
        callbackPass(function(patterns) {
          chrome.test.assertTrue(patterns.includes('https://www.yahoo.com/'));
          chrome.test.assertEq(
              chrome.declarativeNetRequest.MAX_NUMBER_OF_WHITELISTED_PAGES,
              patterns.length, 'Incorrect number of patterns observed.');
        }));
  },

  function removePattern() {
    chrome.declarativeNetRequest.removeWhitelistedPages(
        ['https://www.yahoo.com/'], callbackPass(function() {}));
  },

  function addPattern() {
    // Adding a pattern should now succeed since removing the pattern caused us
    // to go under the limit.
    chrome.declarativeNetRequest.addWhitelistedPages(
        ['https://www.example.com/'], callbackPass(function() {}));
  },

  function verifyPatterns() {
    chrome.declarativeNetRequest.getWhitelistedPages(
        callbackPass(function(patterns) {
          chrome.test.assertTrue(patterns.includes('https://www.example.com/'));
          chrome.test.assertEq(
              chrome.declarativeNetRequest.MAX_NUMBER_OF_WHITELISTED_PAGES,
              patterns.length, 'Incorrect number of patterns observed.');
        }));
  }
]);
