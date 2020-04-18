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
goog.provide('i18n.input.chrome.inputview.layouts.RowsOfNumberpad');
goog.require('i18n.input.chrome.inputview.layouts.util');


/**
 * Creates the rows for compact numberpad keyboard.
 *
 * @return {!Array.<!Object>} The rows.
 */
i18n.input.chrome.inputview.layouts.RowsOfNumberpad.create = function() {
  var baseKeySpec = {
    'widthInWeight': 1,
    'heightInWeight': 0.75
  };

  var smallKeySpec = {
    'widthInWeight': 0.75,
    'heightInWeight': 0.75
  };

  // Row1
  var smallkeySequenceOf3 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(smallKeySpec, 3);
  var keySequenceOf3 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(baseKeySpec, 3);
  var backspaceKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.2,
    'heightInWeight': 0.75
  });
  var row1 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row1',
    'children': [smallkeySequenceOf3, keySequenceOf3, backspaceKey]
  });

  // Row2
  smallkeySequenceOf3 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(smallKeySpec, 3);
  keySequenceOf3 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(baseKeySpec, 3);
  var enterKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.2,
    'heightInWeight': 0.75
  });
  var row2 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row2',
    'children': [smallkeySequenceOf3, keySequenceOf3, enterKey]
  });

  // Row3
  smallkeySequenceOf3 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(smallKeySpec, 3);
  var keySequenceOf2 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(baseKeySpec, 2);
  var rightKeyWithPadding = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 2.2,
    'heightInWeight': 0.75
  });
  var row3 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row3',
    'children': [smallkeySequenceOf3, keySequenceOf2, rightKeyWithPadding]
  });

  // Row4
  var spaceKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 2.25,
    'heightInWeight': 0.75
  });
  keySequenceOf3 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(baseKeySpec, 3);
  var hideKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.2,
    'heightInWeight': 0.75
  });
  var row4 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row4',
    'children': [spaceKey, keySequenceOf3, hideKey]
  });

  return [row1, row2, row3, row4];
};
