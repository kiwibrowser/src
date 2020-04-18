// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @constructor
 * @extends testing.Test
 */
function LoadTimeDataTest() {}

LoadTimeDataTest.prototype = {
  __proto__: testing.Test.prototype,

  /** @override */
  browsePreload: 'chrome://resources/html/load_time_data.html',
};

TEST_F('LoadTimeDataTest', 'sanitizeInnerHtml', function() {
  // A few tests to see that that data is being passed through. The
  // sanitizeInnerHtml() function calls into parseHtmlSubset() which has its
  // own tests (that don't need to be repeated here).
  assertEquals(
      '<a href="chrome://foo"></a>',
      loadTimeData.sanitizeInnerHtml('<a href="chrome://foo"></a>'));
  assertThrows(() => {
    loadTimeData.sanitizeInnerHtml('<div></div>');
  }, 'DIV is not supported');
  assertEquals(
      '<div></div>',
      loadTimeData.sanitizeInnerHtml('<div></div>', {tags: ['div']}));
});

TEST_F('LoadTimeDataTest', 'getStringPieces', function() {
  function assertSubstitutedPieces(expected, var_args) {
    var var_args = Array.prototype.slice.call(arguments, 1);
    var pieces =
        loadTimeData.getSubstitutedStringPieces.apply(loadTimeData, var_args);
    assertDeepEquals(expected, pieces);

    // Ensure output matches getStringF.
    assertEquals(
        loadTimeData.substituteString.apply(loadTimeData, var_args),
        pieces.map(p => p.value).join(''));
  }

  assertSubstitutedPieces([{value: 'paper', arg: null}], 'paper');
  assertSubstitutedPieces([{value: 'paper', arg: '$1'}], '$1', 'paper');

  assertSubstitutedPieces(
      [
        {value: 'i think ', arg: null},
        {value: 'paper mario', arg: '$1'},
        {value: ' is a good game', arg: null},
      ],
      'i think $1 is a good game', 'paper mario');

  assertSubstitutedPieces(
      [
        {value: 'paper mario', arg: '$1'},
        {value: ' costs $', arg: null},
        {value: '60', arg: '$2'},
      ],
      '$1 costs $$$2', 'paper mario', '60');

  assertSubstitutedPieces(
      [
        {value: 'paper mario', arg: '$1'},
        {value: ' costs $60', arg: null},
      ],
      '$1 costs $$60', 'paper mario');

  assertSubstitutedPieces(
      [
        {value: 'paper mario', arg: '$1'},
        {value: ' costs\n$60 ', arg: null},
        {value: 'today', arg: '$2'},
      ],
      '$1 costs\n$$60 $2', 'paper mario', 'today');

  assertSubstitutedPieces(
      [
        {value: '$$', arg: null},
        {value: '1', arg: '$1'},
        {value: '2', arg: '$2'},
        {value: '1', arg: '$1'},
        {value: '$$2', arg: null},
        {value: '2', arg: '$2'},
        {value: '$', arg: null},
        {value: '1', arg: '$1'},
        {value: '$', arg: null},
      ],
      '$$$$$1$2$1$$$$2$2$$$1$$', '1', '2');
});

TEST_F('LoadTimeDataTest', 'unescapedDollarSign', function() {
  function assertSubstitutionThrows(label) {
    assertThrows(() => {
      loadTimeData.getSubstitutedStringPieces(label);
    }, 'Assertion failed: Unescaped $ found in localized string.');

    assertThrows(() => {
      loadTimeData.substituteString(label);
    }, 'Assertion failed: Unescaped $ found in localized string.');
  }
  assertSubstitutionThrows('$');
  assertSubstitutionThrows('$1$$$a2');
  assertSubstitutionThrows('$$$');
  assertSubstitutionThrows('a$');
  assertSubstitutionThrows('a$\n');
});
