// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var pass = chrome.test.callbackPass;
var fail = chrome.test.callbackFail;

function getTestFunctionFor(keys, fails) {
  return function generatedTest () {
    // Debug.
    console.warn('keys: ' + keys + '; fails: ' + fails);

    chrome.chromeosInfoPrivate.get(
        keys,
        pass(
          function(values) {
            for (var i = 0; i < keys.length; ++i) {
              // Default session type should be normal.
              if (keys[i] == 'sessionType') {
                chrome.test.assertEq('normal', values[keys[i]]);
              }
              // PlayStoreStatus by default should be not available.
              if (keys[i] == 'playStoreStatus') {
                chrome.test.assertEq('not available', values[keys[i]]);
              }
              if (keys[i] == 'managedDeviceStatus') {
                chrome.test.assertEq('not managed', values[keys[i]]);
              }
              // Debug
              if (keys[i] in values) {
                console.log('  values["' + keys[i] + '"] = ' +
                            values[keys[i]]);
              } else {
                console.log('  ' + keys[i] + ' is missing in values');
              }

              chrome.test.assertEq(fails.indexOf(keys[i]) == -1,
                                   keys[i] in values);
            }
          }
        )
    );
  }
}

// Automatically generates tests for the given possible keys. Note, this
// tests do not check return value, only the fact that it is presented.
function generateTestsForKeys(keys) {
  var tests = [];
  // Test with all the keys at one.
  tests.push(getTestFunctionFor(keys, []));
  // Tests with key which hasn't corresponding value.
  var noValueKey = 'noValueForThisKey';
  tests.push(getTestFunctionFor([noValueKey], [noValueKey]));

  if (keys.length > 1) {
    // Tests with the separate keys.
    for (var i = 0; i < keys.length; ++i) {
      tests.push(getTestFunctionFor([keys[i]], []));
    }
  }
  if (keys.length >= 2) {
    tests.push(getTestFunctionFor([keys[0], keys[1]], []));
    tests.push(getTestFunctionFor([keys[0], noValueKey, keys[1]],
                                  [noValueKey]));
  }
  return tests;
}

function timezoneSetTest() {
  chrome.chromeosInfoPrivate.set('timezone', 'Pacific/Kiritimati');
  chrome.chromeosInfoPrivate.get(
      ['timezone'],
      pass(
        function(values) {
          chrome.test.assertEq(values['timezone'],
                               'Pacific/Kiritimati');
        }
      ));
}

function prefsTest() {
  chrome.chromeosInfoPrivate.set('a11yLargeCursorEnabled', true);
  chrome.chromeosInfoPrivate.set('a11yStickyKeysEnabled', true);
  chrome.chromeosInfoPrivate.set('a11ySpokenFeedbackEnabled', true);
  chrome.chromeosInfoPrivate.set('a11yHighContrastEnabled', true);
  chrome.chromeosInfoPrivate.set('a11yScreenMagnifierEnabled', true);
  chrome.chromeosInfoPrivate.set('a11yAutoClickEnabled', true);
  chrome.chromeosInfoPrivate.set('a11yVirtualKeyboardEnabled', true);
  chrome.chromeosInfoPrivate.set('sendFunctionKeys', true);
  chrome.chromeosInfoPrivate.get(
      ['a11yLargeCursorEnabled',
       'a11yStickyKeysEnabled',
       'a11ySpokenFeedbackEnabled',
       'a11yHighContrastEnabled',
       'a11yScreenMagnifierEnabled',
       'a11yAutoClickEnabled',
       'a11yVirtualKeyboardEnabled',
       'sendFunctionKeys'],
      pass(
        function(values) {
          chrome.test.assertEq(values['a11yLargeCursorEnabled'], true);
          chrome.test.assertEq(values['a11yStickyKeysEnabled'], true);
          chrome.test.assertEq(values['a11ySpokenFeedbackEnabled'], true);
          chrome.test.assertEq(values['a11yHighContrastEnabled'], true);
          chrome.test.assertEq(values['a11yScreenMagnifierEnabled'], true);
          chrome.test.assertEq(values['a11yAutoClickEnabled'], true);
          chrome.test.assertEq(values['a11yVirtualKeyboardEnabled'], true);
          chrome.test.assertEq(values['sendFunctionKeys'], true);
        }
      ));
}

// Run generated chrome.chromeosInfoPrivate.get() tests.
var tests = generateTestsForKeys(['hwid',
                                  'customizationId',
                                  'homeProvider',
                                  'initialLocale',
                                  'board',
                                  'isOwner',
                                  'sessionType',
                                  'playStoreStatus',
                                  'managedDeviceStatus',
                                  'clientId',
                                  'a11yLargeCursorEnabled',
                                  'a11yStickyKeysEnabled',
                                  'a11ySpokenFeedbackEnabled',
                                  'a11yHighContrastEnabled',
                                  'a11yScreenMagnifierEnabled',
                                  'a11yAutoClickEnabled',
                                  'a11yVirtualKeyboardEnabled',
                                  'sendFunctionKeys',
                                  'timezone',
                                  'supportedTimezones'])

// Add chrome.chromeosInfoPrivate.set() test.
tests.push(timezoneSetTest);
tests.push(prefsTest);

chrome.test.runTests(tests);
