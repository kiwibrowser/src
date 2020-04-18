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
goog.require('i18n.input.chrome.inputview.ConditionName');
goog.require('i18n.input.chrome.inputview.layouts.RowsOf101');
goog.require('i18n.input.chrome.inputview.layouts.util');


(function() {
  var ConditionName = i18n.input.chrome.inputview.ConditionName;
  var RowsOf101 = i18n.input.chrome.inputview.layouts.RowsOf101;
  var util = i18n.input.chrome.inputview.layouts.util;

  util.setPrefix('kokbd-k-');

  /**
   * Creates the spaceKey row.
   *
   * @return {!Object} The spaceKey row.
   */
  var createSpaceRow = function() {
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
    // Creates the Hangja switcher key in the end and insert it before the
    // Space key.
    var hangjaSwitcher = util.createKey({
      'widthInWeight': 1
    });
    var spaceKey = util.createKey({
      'widthInWeight': 4.87
    });
    var enSwitcher = util.createKey({
      'widthInWeight': 1,
      'condition': ConditionName.SHOW_EN_SWITCHER_KEY
    });
    var altGrKey = util.createKey({
      'widthInWeight': 1.25,
      'condition': ConditionName.SHOW_ALTGR
    });
    // If globeKey or altGrKey is not shown, give its weight to space key.
    globeKey['spec']['giveWeightTo'] = spaceKey['spec']['id'];
    menuKey['spec']['giveWeightTo'] = spaceKey['spec']['id'];
    altGrKey['spec']['giveWeightTo'] = spaceKey['spec']['id'];
    hangjaSwitcher['spec']['giveWeightTo'] = spaceKey['spec']['id'];

    var leftKey = util.createKey({
      'widthInWeight': 1.08
    });
    var rightKey = util.createKey({
      'widthInWeight': 1.08
    });
    var hideKeyboardKey = util.createKey({
      'widthInWeight': 1.08
    });
    var spaceKeyRow = util.createLinearLayout({
      'id': 'spaceKeyrow',
      'children': [ctrlKey, altKey, globeKey, menuKey, hangjaSwitcher,
        spaceKey, enSwitcher, altGrKey, leftKey, rightKey,
        hideKeyboardKey]
    });
    return spaceKeyRow;
  };

  var topFourRows = RowsOf101.create();
  var spaceRow = createSpaceRow();

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
    'layoutID': 'kokbd',
    'widthInWeight': 15,
    'children': [keyboardContainer]
  };

  google.ime.chrome.inputview.onLayoutLoaded(data);

}) ();
