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
goog.require('i18n.input.chrome.inputview.layouts.util');


(function() {
  var util = i18n.input.chrome.inputview.layouts.util;
  util.setPrefix('handwriting-k-');

  var verticalRows = [];
  var baseKeySpec = {
    'widthInWeight': 1,
    'heightInWeight': 1
  };
  for (var i = 0; i < 4; i++) {
    verticalRows.push(util.createKey(baseKeySpec));
  }
  var leftSideColumn = util.createVerticalLayout({
    'id': 'leftSideColumn',
    'children': verticalRows
  });

  verticalRows = [];
  for (var i = 0; i < 4; i++) {
    verticalRows.push(util.createKey(baseKeySpec));
  }
  var rightSideColumn = util.createVerticalLayout({
    'id': 'rightSideColumn',
    'children': verticalRows
  });

  var spec = {
    'id': 'canvasView',
    'widthInWeight': 11.2,
    'heightInWeight': 4
  };

  var canvasView = util.createCanvasView(spec);
  var panelView = util.createHandwritingLayout({
    'id': 'panelView',
    'children': [leftSideColumn, canvasView, rightSideColumn]
  });

  // Keyboard view.
  var keyboardView = util.createLayoutView({
    'id': 'keyboardView',
    'children': [panelView],
    'widthPercent': 100,
    'heightPercent': 100
  });


  var keyboardContainer = util.createLinearLayout({
    'id': 'keyboardContainer',
    'children': [keyboardView]
  });

  var data = {
    'layoutID': 'handwriting',
    'heightPercentOfWidth': 0.275,
    'minimumHeight': 350,
    'fullHeightInWeight': 5.6,
    'children': [keyboardContainer]
  };

  google.ime.chrome.inputview.onLayoutLoaded(data);

}) ();
