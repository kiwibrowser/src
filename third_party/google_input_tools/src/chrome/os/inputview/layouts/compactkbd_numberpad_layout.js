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
goog.require('i18n.input.chrome.inputview.layouts.RowsOfNumberpad');
goog.require('i18n.input.chrome.inputview.layouts.util');


(function() {
  i18n.input.chrome.inputview.layouts.util.keyIdPrefix = 'compactkbd-k-';

  var rows = i18n.input.chrome.inputview.layouts.RowsOfNumberpad.create();

  // Keyboard view.
  var keyboardView = i18n.input.chrome.inputview.layouts.util.createLayoutView({
    'id': 'keyboardView',
    'children': [rows],
    'widthPercent': 50,
    'heightPercent': 100
  });

  var keyboardContainer = i18n.input.chrome.inputview.layouts.util.
      createLinearLayout({
        'id': 'keyboardContainer',
        'children': [keyboardView]
      });

  var data = {
    'layoutID': 'compactkbd-numberpad',
    'widthInWeight': 6.45,
    'children': [keyboardContainer],
    'disableCandidateView': true,
    'disableLongpress': true,
    'widthPercent' : {
      'LANDSCAPE' : 0.56,
      'PORTRAIT' : 0.56,
      'LANDSCAPE_WIDE_SCREEN': 0.56
    }};

  google.ime.chrome.inputview.onLayoutLoaded(data);

}) ();
