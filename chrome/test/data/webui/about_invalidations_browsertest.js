// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * TestFixture for Invalidations WebUI testing.
 * @extends {testing.Test}
 * @constructor
 */
function InvalidationsWebUITest() {}

InvalidationsWebUITest.prototype = {
  __proto__: testing.Test.prototype,

  /**
   * Browse to the Invalidations page.
   */
  browsePreload: 'chrome://invalidations',
  runAccessibilityChecks: false,
  accessibilityIssuesAreErrors: false
};

// Test that registering an invalidations appears properly on the textarea.
TEST_F('InvalidationsWebUITest', 'testRegisteringNewInvalidation', function() {
  var invalidationsLog = $('invalidations-log');
  var invalidation = [
    {isUnknownVersion: 'true', objectId: {name: 'EXTENSIONS', source: 1004}}
  ];
  invalidationsLog.value = '';
  chrome.invalidations.logInvalidations(invalidation);
  var isContained =
      invalidationsLog.value.indexOf(
          'Received Invalidation with type ' +
          '"EXTENSIONS" version "Unknown" with payload "undefined"') != -1;
  expectTrue(isContained, 'Actual log is:' + invalidationsLog.value);

});

// Test that changing the Invalidations Service state appears both in the
// span and in the textarea.
TEST_F('InvalidationsWebUITest', 'testChangingInvalidationsState', function() {
  var invalidationsState = $('invalidations-state');
  var invalidationsLog = $('invalidations-log');
  var newState = 'INVALIDATIONS_ENABLED';
  var newNewState = 'TRANSIENT_INVALIDATION_ERROR';

  chrome.invalidations.updateInvalidatorState(newState);
  var isContainedState =
      invalidationsState.textContent.indexOf('INVALIDATIONS_ENABLED') != -1;
  expectTrue(isContainedState, 'could not change the invalidations text');

  invalidationsLog.value = '';
  chrome.invalidations.updateInvalidatorState(newNewState);
  var isContainedState2 = invalidationsState.textContent.indexOf(
                              'TRANSIENT_INVALIDATION_ERROR') != -1;
  expectTrue(isContainedState2, 'could not change the invalidations text');
  var isContainedLog = invalidationsLog.value.indexOf(
                           'Invalidations service state changed to ' +
                           '"TRANSIENT_INVALIDATION_ERROR"') != -1;
  expectTrue(isContainedLog, 'Actual log is:' + invalidationsLog.value);
});

// Test that objects ids appear on the table.
TEST_F('InvalidationsWebUITest', 'testRegisteringNewIds', function() {
  var newDataType = [
    {name: 'EXTENSIONS', source: 1004, totalCount: 0},
    {name: 'FAVICON_IMAGE', source: 1004, totalCount: 0}
  ];
  var pattern1 = ['Fake', '1004', 'EXTENSIONS', '0', '0', '', '', ''];
  var pattern2 = ['Fake', '1004', 'FAVICON_IMAGE', '0', '0', '', '', ''];
  // Register two objects ID with 'Fake' registrar
  chrome.invalidations.updateIds('Fake', newDataType);
  // Disable the Extensions ObjectId by only sending FAVICON_IMAGE
  newDataType = [{name: 'FAVICON_IMAGE', source: 1004}];
  chrome.invalidations.updateIds('Fake', newDataType);

  // Test that the two patterns are contained in the table.
  var oidTable = $('objectsid-table-container');
  var foundPattern1 = false;
  var foundPattern2 = false;
  for (var row = 0; row < oidTable.rows.length; row++) {
    var pattern1Test = true;
    var pattern2Test = true;
    for (var cell = 0; cell < oidTable.rows[row].cells.length; cell++) {
      pattern1Test = pattern1Test &&
          (pattern1[cell] == oidTable.rows[row].cells[cell].textContent);
      pattern2Test = pattern2Test &&
          (pattern2[cell] == oidTable.rows[row].cells[cell].textContent);
    }
    if (pattern1Test)
      expectEquals('greyed', oidTable.rows[row].className);
    if (pattern2Test)
      expectEquals('content', oidTable.rows[row].className);

    foundPattern1 = foundPattern1 || pattern1Test;
    foundPattern2 = foundPattern2 || pattern2Test;
    if (foundPattern2)
      expectTrue(foundPattern1, 'The entries were not ordererd');
  }
  expectTrue(foundPattern1 && foundPattern2, 'couldn\'t find both objects ids');
});

// Test that registering new handlers appear on the website.
TEST_F('InvalidationsWebUITest', 'testUpdatingRegisteredHandlers', function() {
  function text() {
    return $('registered-handlers').textContent;
  }
  chrome.invalidations.updateHandlers(['FakeApi', 'FakeClient']);
  expectNotEquals(text().indexOf('FakeApi'), -1);
  expectNotEquals(text().indexOf('FakeClient'), -1);

  chrome.invalidations.updateHandlers(['FakeClient']);
  expectEquals(text().indexOf('FakeApi'), -1);
  expectNotEquals(text().indexOf('FakeClient'), -1);
});

// Test that an object showing internal state is correctly displayed.
TEST_F('InvalidationsWebUITest', 'testUpdatingInternalDisplay', function() {
  var newDetailedStatus = {MessagesSent: 1};
  chrome.invalidations.updateDetailedStatus(newDetailedStatus);
  expectEquals($('internal-display').value, '{\n  \"MessagesSent\": 1\n}');
});
