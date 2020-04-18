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
goog.require('i18n.input.chrome.inputview.content.ContextlayoutUtil');
goog.require('i18n.input.chrome.inputview.content.compact.letter');
goog.require('i18n.input.chrome.inputview.content.compact.more');
goog.require('i18n.input.chrome.inputview.content.compact.numberpad');
goog.require('i18n.input.chrome.inputview.content.compact.symbol');
goog.require('i18n.input.chrome.inputview.content.compact.util');
goog.require('i18n.input.chrome.inputview.content.util');
goog.require('i18n.input.chrome.message.ContextType');

(function() {
  var ContextType = i18n.input.chrome.message.ContextType;
  var viewIdPrefix_ = '101kbd-k-';

  var keyCharacters = [
    ['\u0300', '\u0303', '\u0060', '\u007e'], // TLDE
    ['\u0031', '\u0021', '\u00a1', '\u00b9'], // AE01
    ['\u0032', '\u0040', '\u00b2', '\u030b'], // AE02
    ['\u0033', '\u0023', '\u00b3', '\u0304'], // AE03
    ['\u0034', '\u0024', '\u00a4', '\u00a3'], // AE04
    ['\u0035', '\u0025', '\u20ac', '\u0327'], // AE05
    ['\u0036', '\u0302', '\u00bc', '\u005e'], // AE06
    ['\u0037', '\u0026', '\u00bd', '\u031b'], // AE07
    ['\u0038', '\u002a', '\u00be', '\u0328'], // AE08
    ['\u0039', '\u0028', '\u2018', '\u0306'], // AE09
    ['\u0030', '\u0029', '\u2019', '\u030a'], // AE10
    ['\u002d', '\u005f', '\u00a5', '\u0323'], // AE11
    ['\u003d', '\u002b', '\u00d7', '\u00f7'], // AE12
    ['\u0071', '\u0051', '\u00e4', '\u00c4'], // AD01
    ['\u0077', '\u0057', '\u00e5', '\u00c5'], // AD02
    ['\u0065', '\u0045', '\u00e9', '\u00c9'], // AD03
    ['\u0072', '\u0052', '\u00ae', '\u00ae'], // AD04
    ['\u0074', '\u0054', '\u00fe', '\u00de'], // AD05
    ['\u0079', '\u0059', '\u00fc', '\u00dc'], // AD06
    ['\u0075', '\u0055', '\u00fa', '\u00da'], // AD07
    ['\u0069', '\u0049', '\u00ed', '\u00cd'], // AD08
    ['\u006f', '\u004f', '\u00f3', '\u00d3'], // AD09
    ['\u0070', '\u0050', '\u00f6', '\u00d6'], // AD10
    ['\u005b', '\u007b', '\u00ab', '\u201c'], // AD11
    ['\u005d', '\u007d', '\u00bb', '\u201d'], // AD12
    ['\u005c', '\u007c', '\u00ac', '\u00a6'], // BKSL
    ['\u0061', '\u0041', '\u00e1', '\u00c1'], // AC01
    ['\u0073', '\u0053', '\u00df', '\u00a7'], // AC02
    ['\u0064', '\u0044', '\u00f0', '\u00d0'], // AC03
    ['\u0066', '\u0046'], // AC04
    ['\u0067', '\u0047'], // AC05
    ['\u0068', '\u0048'], // AC06
    ['\u006a', '\u004a'], // AC07
    ['\u006b', '\u004b', '\u0153', '\u0152'], // AC08
    ['\u006c', '\u004c', '\u00f8', '\u00d8'], // AC09
    ['\u003b', '\u003a', '\u00b6', '\u00b0'], // AC10
    ['\u0301', '\u0308', '\u0027', '\u0022'], // AC11
    ['\u007a', '\u005a', '\u00e6', '\u00c6'], // AB01
    ['\u0078', '\u0058'], // AB02
    ['\u0063', '\u0043', '\u00a9', '\u00a2'], // AB03
    ['\u0076', '\u0056'], // AB04
    ['\u0062', '\u0042'], // AB05
    ['\u006e', '\u004e', '\u00f1', '\u00d1'], // AB06
    ['\u006d', '\u004d', '\u00b5', '\u00b5'], // AB07
    ['\u002c', '\u003c', '\u00e7', '\u00c7'], // AB08
    ['\u002e', '\u003e', '\u0307', '\u030c'], // AB09
    ['\u002f', '\u003f', '\u00bf', '\u0309'], // AB10
    ['\u0020', '\u0020'] // SPCE
  ];

  var keyCodes = [
    0xC0, // TLDE
    0x31, // AE01
    0x32, // AE02
    0x33, // AE03
    0x34, // AE04
    0x35, // AE05
    0x36, // AE06
    0x37, // AE07
    0x38, // AE08
    0x39, // AE09
    0x30, // AE10
    0xBD, // AE11
    0xBB, // AE12
    0x51, // AD01
    0x57, // AD02
    0x45, // AD03
    0x52, // AD04
    0x54, // AD05
    0x59, // AD06
    0x55, // AD07
    0x49, // AD08
    0x4F, // AD09
    0x50, // AD10
    0xDB, // AD11
    0xDD, // AD12
    0xDC, // BKSL
    0x41, // AC01
    0x53, // AC02
    0x44, // AC03
    0x46, // AC04
    0x47, // AC05
    0x48, // AC06
    0x4A, // AC07
    0x4B, // AC08
    0x4C, // AC09
    0xBA, // AC10
    0xDE, // AC11
    0x5A, // AB01
    0x58, // AB02
    0x43, // AB03
    0x56, // AB04
    0x42, // AB05
    0x4E, // AB06
    0x4D, // AB07
    0xBC, // AB08
    0xBE, // AB09
    0xBF, // AB10
    0x20  // SPCE
  ];

  var data = i18n.input.chrome.inputview.content.util.createData(
      keyCharacters, viewIdPrefix_, false, true, keyCodes, 'nl.compact.qwerty');
  data['id'] = 'nl';
  google.ime.chrome.inputview.onConfigLoaded(data);

  var keysetSpecNode =
      i18n.input.chrome.inputview.content.compact.util.CompactKeysetSpec;
  var letterKeysetSpec = {};
  letterKeysetSpec[keysetSpecNode.ID] = 'nl.compact.qwerty';
  letterKeysetSpec[keysetSpecNode.LAYOUT] = 'compactkbd-qwerty';
  letterKeysetSpec[keysetSpecNode.DATA] = i18n.input.chrome.inputview.content.
      compact.letter.keyNederlandCharacters();

  var symbolKeysetSpec = {};
  symbolKeysetSpec[keysetSpecNode.ID] = 'nl.compact.symbol';
  symbolKeysetSpec[keysetSpecNode.LAYOUT] = 'compactkbd-qwerty';
  symbolKeysetSpec[keysetSpecNode.DATA] = i18n.input.chrome.inputview.content.
      compact.symbol.keyEUSymbolCharacters();

  var moreKeysetSpec = {};
  moreKeysetSpec[keysetSpecNode.ID] = 'nl.compact.more';
  moreKeysetSpec[keysetSpecNode.LAYOUT] = 'compactkbd-qwerty';
  moreKeysetSpec[keysetSpecNode.DATA] =
      i18n.input.chrome.inputview.content.compact.more.keyEUMoreCharacters();

  i18n.input.chrome.inputview.content.compact.util.generateCompactKeyboard(
      letterKeysetSpec, symbolKeysetSpec, moreKeysetSpec,
      google.ime.chrome.inputview.onConfigLoaded);

  var inputTypeToKeysetSpecMap = {};

  var numberKeysetSpec = {};
  numberKeysetSpec[keysetSpecNode.ID] = 'nl.compact.numberpad';
  numberKeysetSpec[keysetSpecNode.LAYOUT] = 'compactkbd-numberpad';
  numberKeysetSpec[keysetSpecNode.DATA] = i18n.input.chrome.inputview.content.
      compact.numberpad.keyNumberpadCharacters();
  inputTypeToKeysetSpecMap[ContextType.NUMBER] = numberKeysetSpec;

  var phoneKeysetSpec = {};
  phoneKeysetSpec[keysetSpecNode.ID] = 'nl.compact.phonepad';
  phoneKeysetSpec[keysetSpecNode.LAYOUT] = 'compactkbd-numberpad';
  phoneKeysetSpec[keysetSpecNode.DATA] = i18n.input.chrome.inputview.content.
      compact.numberpad.keyPhonepadCharacters();
  inputTypeToKeysetSpecMap[ContextType.PHONE] = phoneKeysetSpec;

  i18n.input.chrome.inputview.content.ContextlayoutUtil.generateContextLayouts(
      inputTypeToKeysetSpecMap, google.ime.chrome.inputview.onConfigLoaded);

}) ();
