// Copyright 2015 The ChromeOS IME Authors. All Rights Reserved.
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
goog.provide('i18n.input.chrome.inputview.layouts.material.RowsOfCompact');

goog.require('i18n.input.chrome.inputview.layouts.material.util');

goog.scope(function() {
var material = i18n.input.chrome.inputview.layouts.material;


/**
 * Creates the top three rows for compact qwerty keyboard.
 *
 * @return {!Array.<!Object>} The rows.
 */
material.RowsOfCompact.create = function() {
  var baseKeySpec = {
    'widthInWeight': 1,
    'heightInWeight': 1
  };

  // Row1
  var keySequenceOf10 = material.util.createKeySequence(baseKeySpec, 10);
  var backspaceKey = material.util.createKey({
    'widthInWeight': 1.15625
  });
  var row1 = material.util.createLinearLayout({
    'id': 'row1',
    'children': [keySequenceOf10, backspaceKey]
  });

  // Row2
  // How to add padding
  var leftKeyWithPadding = material.util.createKey({
    'widthInWeight': 1.5
  });
  var keySequenceOf8 = material.util.createKeySequence(baseKeySpec, 8);
  var enterKey = material.util.createKey({
    'widthInWeight': 1.65625
  });
  var row2 = material.util.createLinearLayout({
    'id': 'row2',
    'children': [leftKeyWithPadding, keySequenceOf8, enterKey]
  });

  // Row3
  var shiftLeftKey = material.util.createKey({
    'widthInWeight': 1.078125
  });
  var keySequenceOf9 = material.util.createKeySequence(baseKeySpec, 9);
  var shiftRightKey = material.util.createKey({
    'widthInWeight': 1.078125
  });
  var row3 = material.util.createLinearLayout({
    'id': 'row3',
    'children': [shiftLeftKey, keySequenceOf9, shiftRightKey]
  });

  return [row1, row2, row3];
};

});  // goog.scope
