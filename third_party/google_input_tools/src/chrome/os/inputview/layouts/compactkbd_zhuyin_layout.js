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
goog.require('i18n.input.chrome.inputview.layouts.RowsOfCompactZhuyin');
goog.require('i18n.input.chrome.inputview.layouts.util');


(function() {
  i18n.input.chrome.inputview.layouts.util.setPrefix('compactkbd-k-');

  var topRows =
      i18n.input.chrome.inputview.layouts.RowsOfCompactZhuyin.create();

  var digitSwitcher = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.1,
    'heightInWeight': 4
  });
  var globeOrSymbolKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'condition': i18n.input.chrome.inputview.ConditionName.SHOW_GLOBE_OR_SYMBOL,
    'widthInWeight': 1,
    'heightInWeight': 4
  });
  var menuKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'condition': i18n.input.chrome.inputview.ConditionName.SHOW_MENU,
    'widthInWeight': 1,
    'heightInWeight': 4
  });
  var comma = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1,
    'heightInWeight': 4
  });
  var space = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 3,
    'heightInWeight': 4
  });
  var character = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1,
    'heightInWeight': 4
  });
  var period = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1,
    'heightInWeight': 4
  });
  var switcher = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1,
    'heightInWeight': 4
  });
  var hide = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.1,
    'heightInWeight': 4
  });
  menuKey['spec']['giveWeightTo'] = space['spec']['id'];
  globeOrSymbolKey['spec']['giveWeightTo'] = space['spec']['id'];

  var spaceRow = i18n.input.chrome.inputview.layouts.util.
      createLinearLayout({
        'id': 'spaceKeyrow',
        'children': [digitSwitcher, globeOrSymbolKey, menuKey, comma,
          space, character, period, switcher, hide]
      });

  // Keyboard view.
  var keyboardView = i18n.input.chrome.inputview.layouts.util.createLayoutView({
    'id': 'keyboardView',
    'children': [topRows, spaceRow],
    'widthPercent': 100,
    'heightPercent': 100
  });

  var keyboardContainer = i18n.input.chrome.inputview.layouts.util.
      createLinearLayout({
        'id': 'keyboardContainer',
        'children': [keyboardView]
      });

  var data = {
    'layoutID': 'compactkbd-zhuyin',
    'widthInWeight': 15,
    'children': [keyboardContainer]
  };
  google.ime.chrome.inputview.onLayoutLoaded(data);
}) ();
