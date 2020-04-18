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
goog.require('i18n.input.chrome.ElementType');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.SpecNodeName');
goog.require('i18n.input.chrome.inputview.content.util');

(function() {
  var util = i18n.input.chrome.inputview.content.util;
  var ElementType = i18n.input.chrome.ElementType;
  var SpecNodeName = i18n.input.chrome.inputview.SpecNodeName;
  var Css = i18n.input.chrome.inputview.Css;

  var viewIdPrefix = 'handwriting-k-';

  var spec = {};
  spec[SpecNodeName.ID] = 'FullHwtPlaceHolder';
  var placeHolderKey = util.createKey(spec);

  spec[SpecNodeName.ID] = 'Comma';
  spec[SpecNodeName.TYPE] = ElementType.CHARACTER_KEY;
  spec[SpecNodeName.CHARACTERS] = [','];
  var commaKey = util.createKey(spec);

  spec = {};
  spec[SpecNodeName.ID] = 'Period';
  spec[SpecNodeName.TYPE] = ElementType.CHARACTER_KEY;
  spec[SpecNodeName.CHARACTERS] = ['.'];
  var periodKey = util.createKey(spec);

  spec = {};
  spec[SpecNodeName.TEXT] = '';
  spec[SpecNodeName.ICON_CSS_CLASS] = Css.SPACE_ICON;
  spec[SpecNodeName.TYPE] = ElementType.SPACE_KEY;
  spec[SpecNodeName.ID] = 'Space';
  var spaceKey = i18n.input.chrome.inputview.content.util.createKey(spec);

  var keyList = [
    placeHolderKey,
    commaKey,
    periodKey,
    util.createBackToKeyboardKey(),
    util.createBackspaceKey(),
    util.createEnterKey(),
    spaceKey,
    util.createHideKeyboardKey()
  ];

  var mapping = {};
  for (var i = 0; i < keyList.length; i++) {
    var key = keyList[i];
    mapping[key['spec'][SpecNodeName.ID]] = viewIdPrefix + i;
  }

  var result = {};
  result[SpecNodeName.KEY_LIST] = keyList;
  result[SpecNodeName.MAPPING] = mapping;
  result[SpecNodeName.LAYOUT] = 'handwriting';
  result[SpecNodeName.HAS_ALTGR_KEY] = false;
  result['id'] = 'hwt';
  google.ime.chrome.inputview.onConfigLoaded(result);
}) ();
