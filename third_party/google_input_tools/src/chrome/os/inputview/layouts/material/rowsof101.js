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
goog.provide('i18n.input.chrome.inputview.layouts.material.RowsOf101');

goog.require('i18n.input.chrome.inputview.layouts.material.util');


goog.scope(function() {
var material = i18n.input.chrome.inputview.layouts.material;


/**
 * Creates the top four rows for 101 keyboard.
 *
 * @return {!Array.<!Object>} The rows.
 */
material.RowsOf101.create = function() {
  var baseKeySpec = {
    'widthInWeight': 1,
    'heightInWeight': 1
  };

  var keySequenceOf13 = material.util.
      createKeySequence(baseKeySpec, 13);
  var backspaceKey = material.util.createKey({
    'widthInWeight': 1.46
  });
  var row1 = material.util.createLinearLayout({
    'id': 'row1',
    'children': [keySequenceOf13, backspaceKey]
  });

  // Row2
  var tabKey = material.util.createKey({
    'widthInWeight': 1.46
  });
  keySequenceOf13 = material.util.
      createKeySequence(baseKeySpec, 13);
  var row2 = material.util.createLinearLayout({
    'id': 'row2',
    'children': [tabKey, keySequenceOf13]
  });

  // Row3
  var capslockKey = material.util.createKey({
    'widthInWeight': 1.76
  });
  var keySequenceOf11 = material.util.
      createKeySequence(baseKeySpec, 11);
  var enterKey = material.util.createKey({
    'widthInWeight': 1.7
  });
  var row3 = material.util.createLinearLayout({
    'id': 'row3',
    'children': [capslockKey, keySequenceOf11, enterKey]
  });

  // Row4
  var shiftLeftKey = material.util.createKey({
    'widthInWeight': 2.23
  });
  var keySequenceOf10 = material.util.
      createKeySequence(baseKeySpec, 10);
  var shiftRightKey = material.util.createKey({
    'widthInWeight': 2.23
  });
  var row4 = material.util.createLinearLayout({
    'id': 'row4',
    'children': [shiftLeftKey, keySequenceOf10, shiftRightKey]
  });

  return [row1, row2, row3, row4];
};

});  // goog.scope
