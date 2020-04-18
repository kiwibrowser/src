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

goog.require('i18n.input.chrome.inputview.layouts.material.RowsOf101');
goog.require('i18n.input.chrome.inputview.layouts.material.SpaceRow');
goog.require('i18n.input.chrome.inputview.layouts.material.util');

goog.scope(function() {
var material = i18n.input.chrome.inputview.layouts.material;

(function() {
  material.util.setPrefix('101kbd-k-');

  var topFourRows = material.RowsOf101.create();
  var spaceRow = material.SpaceRow.create();

  // Keyboard view.
  var keyboardView = material.util.createLayoutView({
    'id': 'keyboardView',
    'children': [topFourRows, spaceRow],
    'widthPercent': 100,
    'heightPercent': 100
  });

  var keyboardContainer = material.util.
      createLinearLayout({
        'id': 'keyboardContainer',
        'children': [keyboardView]
      });

  var data = {
    'layoutID': 'm-101kbd',
    'widthInWeight': 15,
    'children': [keyboardContainer]
  };

  google.ime.chrome.inputview.onLayoutLoaded(data);

}) ();

});  // goog.scope
