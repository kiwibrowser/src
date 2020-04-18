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
goog.require('i18n.input.chrome.inputview.layouts.CompactSpaceRow');
goog.require('i18n.input.chrome.inputview.layouts.RowsOfCompactNordic');
goog.require('i18n.input.chrome.inputview.layouts.util');


(function() {
  i18n.input.chrome.inputview.layouts.util.setPrefix('compactkbd-k-');

  var topThreeRows =
      i18n.input.chrome.inputview.layouts.RowsOfCompactNordic.create();
  var spaceRow =
      i18n.input.chrome.inputview.layouts.CompactSpaceRow.create(true);

  // Keyboard view.
  var keyboardView = i18n.input.chrome.inputview.layouts.util.createLayoutView({
    'id': 'keyboardView',
    'children': [topThreeRows, spaceRow],
    'widthPercent': 100,
    'heightPercent': 100
  });

  var keyboardContainer = i18n.input.chrome.inputview.layouts.util.
      createLinearLayout({
        'id': 'keyboardContainer',
        'children': [keyboardView]
      });

  var data = {
    'layoutID': 'compactkbd-nordic',
    'widthInWeight': 15,
    'children': [keyboardContainer]
  };

  google.ime.chrome.inputview.onLayoutLoaded(data);

}) ();
