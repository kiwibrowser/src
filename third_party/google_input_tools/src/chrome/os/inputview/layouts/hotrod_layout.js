// Copyright 2016 The ChromeOS IME Authors. All Rights Reserved.
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
goog.require('i18n.input.chrome.inputview.layouts.util');


(function() {
  var util = i18n.input.chrome.inputview.layouts.util;

  util.setPrefix('hotrod-k-');

  var baseKeySpec = {
    'widthInWeight': 1,
    'heightInWeight': 1
  };
  var keyQ_P = util.createKeySequence(baseKeySpec, 10);
  var keyBackspace = util.createKey({
    'widthInWeight': 1.5
  });
  var key7_9 = util.createKeySequence(baseKeySpec, 3);
  var row1 = util.createLinearLayout({
    'id': 'row1',
    'children': [keyQ_P, keyBackspace, key7_9]
  });


  var keyA = util.createKey({
    'widthInWeight': 1.5
  });
  var keyS_L = util.createKeySequence(baseKeySpec, 8);
  var keyEnter = util.createKey({
    'widthInWeight': 2
  });
  var key4_6 = util.createKeySequence(baseKeySpec, 3);
  var row2 = util.createLinearLayout({
    'id': 'row2',
    'children': [keyA, keyS_L, keyEnter, key4_6]
  });

  var keyZ = util.createKey({
    'widthInWeight': 3
  });
  var keyX_N = util.createKeySequence(baseKeySpec, 5);
  var keyM = util.createKey({
    'widthInWeight': 3.5
  });
  var key1_3 = util.createKeySequence(baseKeySpec, 3);
  var row3 = util.createLinearLayout({
    'id': 'row3',
    'children': [keyZ, keyX_N, keyM, key1_3]
  });

  var keyFullKeyboard = util.createKey({
    'widthInWeight': 1.5
  });
  var keySequenceOf8 = util.createKeySequence(baseKeySpec, 8);
  var keyMWithPadding = util.createKey({
    'widthInWeight': 2
  });
  var key0 = util.createKey({
    'widthInWeight': 3
  });
  var row4 = util.createLinearLayout({
    'id': 'row4',
    'children': [keyFullKeyboard, keySequenceOf8, keyMWithPadding, key0]
  });

  var keyboardView = util.createLayoutView({
    'id': 'keyboardView',
    'children': [row1, row2, row3, row4],
    'widthPercent': 100,
    'heightPercent': 100
  });

  var keyboardContainer = util.createLinearLayout({
    'id': 'keyboardContainer',
    'children': [keyboardView]
  });

  var data = {
    'layoutID': 'hotrod',
    'disableCandidateView': true,
    'children': [keyboardContainer]
  };

  google.ime.chrome.inputview.onLayoutLoaded(data);

}) ();
