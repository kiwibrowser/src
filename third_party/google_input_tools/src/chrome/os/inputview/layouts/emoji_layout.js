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
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.layouts.util');

(function() {
  var util = i18n.input.chrome.inputview.layouts.util;
  util.setPrefix('emoji-k-');
  // TODO: we should avoid using hard-coded number.
  var pages = [1, 3, 6, 4, 6, 7, 5, 8];
  var emojiPage = {};
  var emojiList = [];
  var baseKeySpec = {
    'widthInWeight': 1,
    'heightInWeight': 1
  };

  // The top tabbar row.
  var keySequenceOf9 = util.createKeySequence(baseKeySpec, 9);
  var rightKey = util.createKey({
    'widthInWeight': 1.037,
    'heightInWeight': 1
  });
  var tabBar = util.createLinearLayout({
    'id': 'tabBar',
    'children': [keySequenceOf9, rightKey],
    'iconCssClass': i18n.input.chrome.inputview.Css.LINEAR_LAYOUT_BORDER
  });

  // The emoji pages.
  baseKeySpec = {
    'widthInWeight': 1,
    'heightInWeight': 1.67
  };
  var totalPages = 0;
  for (var i = 0; i < pages.length; i++) {
    totalPages += pages[i];
  }
  for (var i = 0; i < totalPages; i++) {
    var rows = [];
    for (var j = 0; j < 3; j++) {
      keySequenceOf9 = util.createKeySequence(baseKeySpec, 9);
      var row = util.createLinearLayout({
        'id': 'page-' + i + '-row-' + j,
        'children': [keySequenceOf9],
        'iconCssClass': i18n.input.chrome.inputview.Css.LINEAR_LAYOUT_BORDER
      });
      rows.push(row);
    }
    emojiPage = util.createVerticalLayout({
      'id': 'page-' + i,
      'children': rows
    });
    emojiList.push(emojiPage);
  }
  var emojiRows = util.createExtendedLayout({
    'id': 'emojiRows',
    'children': emojiList
  });
  var emojiSlider = util.createVerticalLayout({
    'id': 'emojiSlider',
    'children': [emojiRows]
  });

  // The right side keys.
  baseKeySpec = {
    'widthInWeight': 1.037,
    'heightInWeight': 1.67
  };
  var sideKeys = util.createVerticalLayout({
    'id': 'sideKeys',
    'children': [util.createKeySequence(baseKeySpec, 3)]
  });

  var rowsAndSideKeys = util.createLinearLayout({
    'id': 'rowsAndSideKeys',
    'children': [emojiSlider, sideKeys]
  });

  var backToKeyboardKey = util.createKey({
    'widthInWeight': 2,
    'heightInWeight': 1.67
  });
  var spaceKey = util.createKey({
    'widthInWeight': 7,
    'heightInWeight': 1.67
  });
  var hideKeyboardKey = util.createKey({
    'widthInWeight': 1.037,
    'heightInWeight': 1.67
  });
  var spaceRow = util.createLinearLayout({
    'id': 'emojiSpaceRow',
    'children': [backToKeyboardKey, spaceKey, hideKeyboardKey]
  });

  var emojiView = util.createVerticalLayout({
    'id': 'emojiView',
    'children': [tabBar, rowsAndSideKeys, spaceRow]
  });

  // Keyboard view.
  var keyboardView = util.createLayoutView({
    'id': 'keyboardView',
    'children': [emojiView],
    'widthPercent': 100,
    'heightPercent': 100
  });


  var keyboardContainer = util.createLinearLayout({
    'id': 'keyboardContainer',
    'children': [keyboardView]
  });

  var data = {
    'disableCandidateView': true,
    'layoutID': 'emoji',
    'children': [keyboardContainer]
  };

  google.ime.chrome.inputview.onLayoutLoaded(data);
}) ();
