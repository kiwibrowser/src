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

goog.require('i18n.input.chrome.inputview.layouts.RowsOf101');
goog.require('i18n.input.chrome.inputview.layouts.SpaceRow');
goog.require('i18n.input.chrome.inputview.layouts.util');

goog.scope(function() {
var layouts = i18n.input.chrome.inputview.layouts;

(function() {
  layouts.util.setPrefix('101kbd-k-');

  var topFourRows = layouts.RowsOf101.create();
  var spaceRow = layouts.SpaceRow.create();

  // Keyboard view.
  var keyboardView = layouts.util.createLayoutView({
    'id': 'keyboardView',
    'children': [topFourRows, spaceRow],
    'widthPercent': 100,
    'heightPercent': 100
  });

  var keyboardContainer = layouts.util.
      createLinearLayout({
        'id': 'keyboardContainer',
        'children': [keyboardView]
      });

  var data = {
    'layoutID': '101kbd',
    'widthInWeight': 15,
    'children': [keyboardContainer]
  };

  google.ime.chrome.inputview.onLayoutLoaded(data);

}) ();

});  // goog.scope
