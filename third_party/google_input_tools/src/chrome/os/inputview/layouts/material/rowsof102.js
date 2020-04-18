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
goog.provide('i18n.input.chrome.inputview.layouts.material.RowsOf102');

goog.require('i18n.input.chrome.inputview.layouts.material.util');

goog.scope(function() {
var material = i18n.input.chrome.inputview.layouts.material;


/**
 * Creates the top four rows for 102 keyboard.
 *
 * @return {!Array.<!Object>} The rows.
 */
material.RowsOf102.create = function() {
  var baseKeySpec = {
    'widthInWeight': 1,
    'heightInWeight': 1
  };

  // Row1
  var keySequenceOf13 = material.util.createKeySequence(baseKeySpec, 13);
  var backspaceKey = material.util.createKey({
    'widthInWeight': 1.46
  });
  var row1 = material.util.createLinearLayout({
    'id': 'row1',
    'children': [keySequenceOf13, backspaceKey]
  });


  // Row2 and row3

  // First linear layout at the left of the enter.
  var tabKey = material.util.createKey({
    'widthInWeight': 1.46
  });
  var keySequenceOf12 = material.util.createKeySequence(baseKeySpec, 12);
  var row2 = material.util.createLinearLayout({
    'id': 'row2',
    'children': [tabKey, keySequenceOf12]
  });

  // Second linear layout at the right of the enter.
  var capslockKey = material.util.createKey({
    'widthInWeight': 1.66
  });
  keySequenceOf12 = material.util.createKeySequence({
    'widthInWeight': 0.98
  }, 12);
  var row3 = material.util.createLinearLayout({
    'id': 'row3',
    'children': [capslockKey, keySequenceOf12]
  });

  // Vertical layout contains the two rows at the left of the enter.
  var vLayout = material.util.createVerticalLayout({
    'id': 'row2-3-left',
    'children': [row2, row3]
  });

  // Vertical layout contains enter key.
  var enterKey = material.util.createKey({
    'widthInWeight': 1.04,
    'heightInWeight': 2
  });
  var enterLayout = material.util.
      createVerticalLayout({
        'id': 'row2-3-right',
        'children': [enterKey]
      });

  // Linear layout contains the two vertical layout.
  var row2and3 = material.util.createLinearLayout({
    'id': 'row2-3',
    'children': [vLayout, enterLayout]
  });

  // Row4
  var shiftLeft = material.util.createKey({
    'widthInWeight': 1.46
  });
  var keySequenceOf11 = material.util.createKeySequence(baseKeySpec, 11);
  var shiftRight = material.util.createKey({
    'widthInWeight': 2
  });
  var row4 = material.util.createLinearLayout({
    'id': 'row4',
    'children': [shiftLeft, keySequenceOf11, shiftRight]
  });

  return [row1, row2and3, row4];
};

});  // goog.scope
