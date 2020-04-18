// Copyright 2014 The ChromeOS IME Authors. All Rights Reserved.
// limitations under the License.
// See the License for the specific language governing permissions and
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// distributed under the License is distributed on an "AS-IS" BASIS,
// Unless required by applicable law or agreed to in writing, software
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// You may obtain a copy of the License at
// you may not use this file except in compliance with the License.
// Licensed under the Apache License, Version 2.0 (the "License");
//
goog.provide('i18n.input.chrome.inputview.layouts.RowsOfCompact');
goog.provide('i18n.input.chrome.inputview.layouts.RowsOfCompactAzerty');
goog.provide('i18n.input.chrome.inputview.layouts.RowsOfCompactNordic');
goog.provide('i18n.input.chrome.inputview.layouts.RowsOfCompactZhuyin');

goog.require('i18n.input.chrome.inputview.layouts.util');

goog.scope(function() {
var layouts = i18n.input.chrome.inputview.layouts;
var util = layouts.util;


/**
 * Creates the top three rows for compact qwerty keyboard.
 *
 * @return {!Array.<!Object>} The rows.
 */
layouts.RowsOfCompact.create = function() {
  var baseKeySpec = {
    'widthInWeight': 1,
    'heightInWeight': 1
  };

  // Row1
  var keySequenceOf10 = util.createKeySequence(baseKeySpec, 10);
  var backspaceKey = util.createKey({
    'widthInWeight': 1.15625
  });
  var row1 = util.createLinearLayout({
    'id': 'row1',
    'children': [keySequenceOf10, backspaceKey]
  });

  // Row2
  // How to add padding
  var leftKeyWithPadding = util.createKey({
    'widthInWeight': 1.5
  });
  var keySequenceOf8 = util.createKeySequence(baseKeySpec, 8);
  var enterKey = util.createKey({
    'widthInWeight': 1.65625
  });
  var row2 = util.createLinearLayout({
    'id': 'row2',
    'children': [leftKeyWithPadding, keySequenceOf8, enterKey]
  });

  // Row3
  var shiftLeftKey = util.createKey({
    'widthInWeight': 1.078125
  });
  var keySequenceOf9 = util.createKeySequence(baseKeySpec, 9);
  var shiftRightKey = util.createKey({
    'widthInWeight': 1.078125
  });
  var row3 = util.createLinearLayout({
    'id': 'row3',
    'children': [shiftLeftKey, keySequenceOf9, shiftRightKey]
  });

  return [row1, row2, row3];
};


/**
 * Creates the top three rows for compact azerty keyboard.
 *
 * @return {!Array.<!Object>} The rows.
 */
i18n.input.chrome.inputview.layouts.RowsOfCompactAzerty.create = function() {
  var baseKeySpec = {
    'widthInWeight': 1,
    'heightInWeight': 1
  };

  // Row1
  var keySequenceOf10 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(baseKeySpec, 10);
  var backspaceKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.2
  });
  var row1 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row1',
    'children': [keySequenceOf10, backspaceKey]
  });

  // Row2
  keySequenceOf10 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(baseKeySpec, 10);
  var enterKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.2
  });
  var row2 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row2',
    'children': [keySequenceOf10, enterKey]
  });

  // Row3
  var shiftLeftKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.1
  });
  var keySequenceOf9 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(baseKeySpec, 9);
  var shiftRightKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.1
  });
  var row3 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row3',
    'children': [shiftLeftKey, keySequenceOf9, shiftRightKey]
  });

  return [row1, row2, row3];
};


/**
 * Creates the top three rows for compact nordic keyboard.
 *
 * @return {!Array.<!Object>} The rows.
 */
i18n.input.chrome.inputview.layouts.RowsOfCompactNordic.create = function() {
  var baseKeySpec = {
    'widthInWeight': 1,
    'heightInWeight': 1
  };

  // Row1
  var keySequenceOf11 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(baseKeySpec, 11);
  var backspaceKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.2
  });
  var row1 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row1',
    'children': [keySequenceOf11, backspaceKey]
  });

  // Row2
  keySequenceOf11 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(baseKeySpec, 11);
  var enterKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.2
  });
  var row2 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row2',
    'children': [keySequenceOf11, enterKey]
  });

  // Row3
  var shiftLeftKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.1
  });
  var leftKeyWithPadding = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.5
  });
  var keySequenceOf7 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(baseKeySpec, 7);
  var rightKeyWithPadding = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.5
  });
  var shiftRightKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.1
  });
  var row3 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row3',
    'children': [shiftLeftKey, leftKeyWithPadding, keySequenceOf7,
      rightKeyWithPadding, shiftRightKey]
  });
  return [row1, row2, row3];
};


/**
 * Creates the top rows of compact zhuyin.
 *
 * @return {!Array.<!Object>} The rows.
 */
i18n.input.chrome.inputview.layouts.RowsOfCompactZhuyin.create = function() {
  var baseKeySpec = {
    'widthInWeight': 1.01,
    'heightInWeight': 3
  };

  // Row1
  var keysOfRow1 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(baseKeySpec, 10);
  var row1 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row1',
    'children': [keysOfRow1]
  });

  // Row2
  var keysOfRow2 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(baseKeySpec, 10);
  var row2 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row2',
    'children': [keysOfRow2]
  });

  //Row3
  var keysOfRow3 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(baseKeySpec, 10);
  var row3 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row3',
    'children': [keysOfRow3]
  });

  // Row4
  var keysOfRow4 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(baseKeySpec, 10);
  var row4 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row4',
    'children': [keysOfRow4]
  });

  var topFourRows =
      i18n.input.chrome.inputview.layouts.util.createVerticalLayout({
        'id': 'topFourRows',
        'children': [row1, row2, row3, row4]
      });

  var sideKeySpec = {
    'widthInWeight': 1.1,
    'heightInWeight': 4
  };
  var backspaceKey =
      i18n.input.chrome.inputview.layouts.util.createKey(sideKeySpec);
  var enterKey =
      i18n.input.chrome.inputview.layouts.util.createKey(sideKeySpec);
  var shiftKey =
      i18n.input.chrome.inputview.layouts.util.createKey(sideKeySpec);

  var sideKeys = i18n.input.chrome.inputview.layouts.util.createVerticalLayout({
    'id': 'sideKeys',
    'children': [backspaceKey, enterKey, shiftKey]
  });

  var topRows = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'topRows',
    'children': [topFourRows, sideKeys]
  });
  return [topRows];
};

});  // goog.scope
