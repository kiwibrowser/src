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
goog.require('i18n.input.chrome.inputview.ConditionName');
goog.require('i18n.input.chrome.inputview.layouts.RowsOfJP');
goog.require('i18n.input.chrome.inputview.layouts.util');


(function() {
  var ConditionName = i18n.input.chrome.inputview.ConditionName;
  var util = i18n.input.chrome.inputview.layouts.util;
  var RowsOfJP = i18n.input.chrome.inputview.layouts.RowsOfJP;
  util.setPrefix('jpkbd-k-');

  var topFourRows = RowsOfJP.create();
  // Creates the space row.
  var globeKey = util.createKey({
    'condition': ConditionName.SHOW_GLOBE_OR_SYMBOL,
    'widthInWeight': 1
  });
  var menuKey = util.createKey({
    'condition': ConditionName.SHOW_MENU,
    'widthInWeight': 1
  });
  var ctrlKey = util.createKey({
    'widthInWeight': 1
  });
  var altKey = util.createKey({
    'widthInWeight': 1
  });


  var leftIMEKey = util.createKey({'widthInWeight': 1});
  var spaceKey = util.createKey({'widthInWeight': 6});
  var rightIMEKey = util.createKey({'widthInWeight': 1});

  // If globeKey or altGrKey is not shown, give its weight to space key.
  globeKey['spec']['giveWeightTo'] = spaceKey['spec']['id'];
  menuKey['spec']['giveWeightTo'] = spaceKey['spec']['id'];

  var leftKey = util.createKey({
    'widthInWeight': 1
  });
  var rightKey = util.createKey({
    'widthInWeight': 1
  });
  var hideKeyboardKey = util.createKey({
    'widthInWeight': 1
  });

  var keys = [
    ctrlKey,
    altKey,
    globeKey,
    menuKey,
    leftIMEKey,
    spaceKey,
    rightIMEKey,
    leftKey,
    rightKey,
    hideKeyboardKey
  ];

  var spaceRow = util.createLinearLayout({
    'id': 'spaceKeyrow',
    'children': keys
  });


  // Keyboard view.
  var keyboardView = util.createLayoutView({
    'id': 'keyboardView',
    'children': [topFourRows, spaceRow],
    'widthPercent': 100,
    'heightPercent': 100
  });

  var keyboardContainer = util.createLinearLayout({
    'id': 'keyboardContainer',
    'children': [keyboardView]
  });

  var data = {
    'layoutID': 'jpkbd',
    'widthInWeight': 15,
    'children': [keyboardContainer]
  };

  google.ime.chrome.inputview.onLayoutLoaded(data);

}) ();
