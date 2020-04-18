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
goog.provide('i18n.input.chrome.inputview.layouts.material.SpaceRow');

goog.require('i18n.input.chrome.inputview.ConditionName');
goog.require('i18n.input.chrome.inputview.layouts.material.util');


goog.scope(function() {
var material = i18n.input.chrome.inputview.layouts.material;


/**
 * Creates the spaceKey row.
 *
 * @return {!Object} The spaceKey row.
 */
material.SpaceRow.create = function() {
  var globeKey = material.util.createKey({
    'condition': i18n.input.chrome.inputview.ConditionName.SHOW_GLOBE_OR_SYMBOL,
    'widthInWeight': 1
  });
  var menuKey = material.util.createKey({
    'condition': i18n.input.chrome.inputview.ConditionName.SHOW_MENU,
    'widthInWeight': 1
  });
  var ctrlKey = material.util.createKey({
    'widthInWeight': 1
  });
  var altKey = material.util.createKey({
    'widthInWeight': 1
  });
  var spaceKey = material.util.createKey({
    'widthInWeight': 5.21
  });
  var enSwitcher = material.util.createKey({
    'widthInWeight': 1,
    'condition': i18n.input.chrome.inputview.ConditionName.
        SHOW_EN_SWITCHER_KEY
  });
  var altGrKey = material.util.createKey({
    'widthInWeight': 1.25,
    'condition': i18n.input.chrome.inputview.ConditionName.
        SHOW_ALTGR
  });
  // If globeKey or altGrKey is not shown, give its weight to space key.
  globeKey['spec']['giveWeightTo'] = spaceKey['spec']['id'];
  menuKey['spec']['giveWeightTo'] = spaceKey['spec']['id'];
  altGrKey['spec']['giveWeightTo'] = spaceKey['spec']['id'];
  enSwitcher['spec']['giveWeightTo'] = spaceKey['spec']['id'];

  var leftKey = material.util.createKey({
    'widthInWeight': 1
  });
  var rightKey = material.util.createKey({
    'widthInWeight': 1
  });
  var hideKeyboardKey = material.util.createKey({
    'widthInWeight': 1
  });
  var spaceKeyRow = material.util.createLinearLayout({
        'id': 'spaceKeyrow',
        'children': [ctrlKey, altKey, globeKey, menuKey, spaceKey,
      enSwitcher, altGrKey, leftKey, rightKey, hideKeyboardKey]
      });
  return spaceKeyRow;
};

});  // goog.scope
