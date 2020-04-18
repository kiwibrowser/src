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
goog.require('i18n.input.chrome.inputview.content.ContextlayoutUtil');
goog.require('i18n.input.chrome.inputview.content.compact.letter');
goog.require('i18n.input.chrome.inputview.content.compact.more');
goog.require('i18n.input.chrome.inputview.content.compact.numberpad');
goog.require('i18n.input.chrome.inputview.content.compact.symbol');
goog.require('i18n.input.chrome.inputview.content.compact.util');
goog.require('i18n.input.chrome.inputview.content.util');
goog.require('i18n.input.chrome.message.ContextType');

(function() {
  var viewIdPrefix_ = '101kbd-k-';
  var ContextType = i18n.input.chrome.message.ContextType;

  var keyCharacters = [
    ['\u0060', '\u007e'], // TLDE
    ['\u0031', '\u0021'], // AE01
    ['\u0032', '\u0040'], // AE02
    ['\u0033', '\u0023'], // AE03
    ['\u0034', '\u0024'], // AE04
    ['\u0035', '\u0025'], // AE05
    ['\u0036', '\u005e'], // AE06
    ['\u0037', '\u0026'], // AE07
    ['\u0038', '\u002a'], // AE08
    ['\u0039', '\u0028'], // AE09
    ['\u0030', '\u0029'], // AE10
    ['\u002d', '\u005f'], // AE11
    ['\u003d', '\u002b'], // AE12
    ['\u0071', '\u0051'], // AD01
    ['\u0077', '\u0057'], // AD02
    ['\u0065', '\u0045'], // AD03
    ['\u0072', '\u0052'], // AD04
    ['\u0074', '\u0054'], // AD05
    ['\u0079', '\u0059'], // AD06
    ['\u0075', '\u0055'], // AD07
    ['\u0069', '\u0049'], // AD08
    ['\u006f', '\u004f'], // AD09
    ['\u0070', '\u0050'], // AD10
    ['\u005b', '\u007b'], // AD11
    ['\u005d', '\u007d'], // AD12
    ['\u005c', '\u007c'], // BKSL
    ['\u0061', '\u0041'], // AC01
    ['\u0073', '\u0053'], // AC02
    ['\u0064', '\u0044'], // AC03
    ['\u0066', '\u0046'], // AC04
    ['\u0067', '\u0047'], // AC05
    ['\u0068', '\u0048'], // AC06
    ['\u006a', '\u004a'], // AC07
    ['\u006b', '\u004b'], // AC08
    ['\u006c', '\u004c'], // AC09
    ['\u003b', '\u003a'], // AC10
    ['\u0027', '\u0022'], // AC11
    ['\u007a', '\u005a'], // AB01
    ['\u0078', '\u0058'], // AB02
    ['\u0063', '\u0043'], // AB03
    ['\u0076', '\u0056'], // AB04
    ['\u0062', '\u0042'], // AB05
    ['\u006e', '\u004e'], // AB06
    ['\u006d', '\u004d'], // AB07
    ['\u002c', '\u003c'], // AB08
    ['\u002e', '\u003e'], // AB09
    ['\u002f', '\u003f'], // AB10
    ['\u0020', '\u0020'] // SPCE
  ];

  var data = i18n.input.chrome.inputview.content.util.createData(
      keyCharacters, viewIdPrefix_, false, false, undefined,
      'us.compact.qwerty');
  data['id'] = 'us';
  google.ime.chrome.inputview.onConfigLoaded(data);

  var keysetSpecNode =
      i18n.input.chrome.inputview.content.compact.util.CompactKeysetSpec;
  var letterKeysetSpec = {};
  letterKeysetSpec[keysetSpecNode.ID] = 'us.compact.qwerty';
  letterKeysetSpec[keysetSpecNode.LAYOUT] = 'compactkbd-qwerty';
  letterKeysetSpec[keysetSpecNode.DATA] =
      i18n.input.chrome.inputview.content.compact.letter.keyQwertyCharacters();

  var symbolKeysetSpec = {};
  symbolKeysetSpec[keysetSpecNode.ID] = 'us.compact.symbol';
  symbolKeysetSpec[keysetSpecNode.LAYOUT] = 'compactkbd-qwerty';
  symbolKeysetSpec[keysetSpecNode.DATA] = i18n.input.chrome.inputview.content.
      compact.symbol.keyNASymbolCharacters();

  var moreKeysetSpec = {};
  moreKeysetSpec[keysetSpecNode.ID] = 'us.compact.more';
  moreKeysetSpec[keysetSpecNode.LAYOUT] = 'compactkbd-qwerty';
  moreKeysetSpec[keysetSpecNode.DATA] =
      i18n.input.chrome.inputview.content.compact.more.keyNAMoreCharacters();

  i18n.input.chrome.inputview.content.compact.util.generateCompactKeyboard(
      letterKeysetSpec, symbolKeysetSpec, moreKeysetSpec,
      google.ime.chrome.inputview.onConfigLoaded);

  var inputTypeToKeysetSpecMap = {};

  var numberKeysetSpec = {};
  numberKeysetSpec[keysetSpecNode.ID] = 'us.compact.numberpad';
  numberKeysetSpec[keysetSpecNode.LAYOUT] = 'compactkbd-numberpad';
  numberKeysetSpec[keysetSpecNode.DATA] = i18n.input.chrome.inputview.content.
      compact.numberpad.keyNumberpadCharacters();
  inputTypeToKeysetSpecMap[ContextType.NUMBER] = numberKeysetSpec;

  var phoneKeysetSpec = {};
  phoneKeysetSpec[keysetSpecNode.ID] = 'us.compact.phonepad';
  phoneKeysetSpec[keysetSpecNode.LAYOUT] = 'compactkbd-numberpad';
  phoneKeysetSpec[keysetSpecNode.DATA] = i18n.input.chrome.inputview.content.
      compact.numberpad.keyPhonepadCharacters();
  inputTypeToKeysetSpecMap[ContextType.PHONE] = phoneKeysetSpec;

  i18n.input.chrome.inputview.content.ContextlayoutUtil.generateContextLayouts(
      inputTypeToKeysetSpecMap, google.ime.chrome.inputview.onConfigLoaded);
}) ();
