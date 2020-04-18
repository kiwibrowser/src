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
  var ContextType = i18n.input.chrome.message.ContextType;
  var viewIdPrefix_ = '102kbd-k-';

  var keyCharacters = [
    ['\u00a7', '\u00bd', '\u0000', '\u0000'], // TLDE
    ['\u0031', '\u0021', '\u0000', '\u00a1'], // AE01
    ['\u0032', '\u0022', '\u0040', '\u201d'], // AE02
    ['\u0033', '\u0023', '\u00a3', '\u00bb'], // AE03
    ['\u0034', '\u00a4', '\u0024', '\u00ab'], // AE04
    ['\u0035', '\u0025', '\u2030', '\u201c'], // AE05
    ['\u0036', '\u0026', '\u201a', '\u201e'], // AE06
    ['\u0037', '\u002f', '\u007b', '\u0000'], // AE07
    ['\u0038', '\u0028', '\u005b', '\u003c'], // AE08
    ['\u0039', '\u0029', '\u005d', '\u003e'], // AE09
    ['\u0030', '\u003d', '\u007d', '\u00b0'], // AE10
    ['\u002b', '\u003f', '\u005c', '\u00bf'], // AE11
    ['\u0301', '\u0300', '\u0327', '\u0328'], // AE12
    ['\u0071', '\u0051'], // AD01
    ['\u0077', '\u0057'], // AD02
    ['\u0065', '\u0045', '\u20ac', '\u0000'], // AD03
    ['\u0072', '\u0052'], // AD04
    ['\u0074', '\u0054', '\u00fe', '\u00de'], // AD05
    ['\u0079', '\u0059'], // AD06
    ['\u0075', '\u0055'], // AD07
    ['\u0069', '\u0049', '\u0131', '\u007c'], // AD08
    ['\u006f', '\u004f', '\u0153', '\u0152'], // AD09
    ['\u0070', '\u0050', '\u031b', '\u0309'], // AD10
    ['\u00e5', '\u00c5', '\u030b', '\u030a'], // AD11
    ['\u0308', '\u0302', '\u0303', '\u0304'], // AD12
    ['\u0061', '\u0041', '\u0259', '\u018f'], // AC01
    ['\u0073', '\u0053', '\u00df', '\u0000'], // AC02
    ['\u0064', '\u0044', '\u00f0', '\u00d0'], // AC03
    ['\u0066', '\u0046'], // AC04
    ['\u0067', '\u0047'], // AC05
    ['\u0068', '\u0048'], // AC06
    ['\u006a', '\u004a'], // AC07
    ['\u006b', '\u004b', '\u0138', '\u0000'], // AC08
    ['\u006c', '\u004c', '\u0000', '\u0000'], // AC09
    ['\u00f6', '\u00d6', '\u00f8', '\u00d8'], // AC10
    ['\u00e4', '\u00c4', '\u00e6', '\u00c6'], // AC11
    ['\u0027', '\u002a', '\u030c', '\u0306'], // BKSL
    ['\u003c', '\u003e', '\u007c', '\u00a6'], // LSGT
    ['\u007a', '\u005a', '\u0292', '\u01b7'], // AB01
    ['\u0078', '\u0058', '\u00d7', '\u00b7'], // AB02
    ['\u0063', '\u0043'], // AB03
    ['\u0076', '\u0056'], // AB04
    ['\u0062', '\u0042'], // AB05
    ['\u006e', '\u004e', '\u014b', '\u014a'], // AB06
    ['\u006d', '\u004d', '\u00b5', '\u2014'], // AB07
    ['\u002c', '\u003b', '\u2019', '\u2018'], // AB08
    ['\u002e', '\u003a', '\u0323', '\u0307'], // AB09
    ['\u002d', '\u005f', '\u2013', '\u0000'], // AB10
    ['\u0020', '\u0020', '\u00a0', '\u00a0'] // SPCE
  ];

  var keyCodes = [
    0xDC, // TLDE
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
    0xBB, // AE11
    0xDB, // AE12
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
    0xDD, // AD11
    0xBA, // AD12
    0x41, // AC01
    0x53, // AC02
    0x44, // AC03
    0x46, // AC04
    0x47, // AC05
    0x48, // AC06
    0x4A, // AC07
    0x4B, // AC08
    0x4C, // AC09
    0xC0, // AC10
    0xDE, // AC11
    0xBF, // BKSL
    0xE2, // LTGT
    0x5A, // AB01
    0x58, // AB02
    0x43, // AB03
    0x56, // AB04
    0x42, // AB05
    0x4E, // AB06
    0x4D, // AB07
    0xBC, // AB08
    0xBE, // AB09
    0xBD, // AB10
    0x20  // SPCE
  ];

  var data = i18n.input.chrome.inputview.content.util.createData(
      keyCharacters, viewIdPrefix_, true, true, keyCodes, 'fi.compact.qwerty');
  data['id'] = 'fi';
  google.ime.chrome.inputview.onConfigLoaded(data);

  var keysetSpecNode =
      i18n.input.chrome.inputview.content.compact.util.CompactKeysetSpec;
  var letterKeysetSpec = {};
  letterKeysetSpec[keysetSpecNode.ID] = 'fi.compact.qwerty';
  letterKeysetSpec[keysetSpecNode.LAYOUT] = 'compactkbd-nordic';
  letterKeysetSpec[keysetSpecNode.DATA] =
      i18n.input.chrome.inputview.content.compact.letter.keyNordicCharacters();

  var symbolKeysetSpec = {};
  symbolKeysetSpec[keysetSpecNode.ID] = 'fi.compact.symbol';
  symbolKeysetSpec[keysetSpecNode.LAYOUT] = 'compactkbd-qwerty';
  symbolKeysetSpec[keysetSpecNode.DATA] = i18n.input.chrome.inputview.content.
      compact.symbol.keyEUSymbolCharacters();

  var moreKeysetSpec = {};
  moreKeysetSpec[keysetSpecNode.ID] = 'fi.compact.more';
  moreKeysetSpec[keysetSpecNode.LAYOUT] = 'compactkbd-qwerty';
  moreKeysetSpec[keysetSpecNode.DATA] =
      i18n.input.chrome.inputview.content.compact.more.keyEUMoreCharacters();

  i18n.input.chrome.inputview.content.compact.util.generateCompactKeyboard(
      letterKeysetSpec, symbolKeysetSpec, moreKeysetSpec,
      google.ime.chrome.inputview.onConfigLoaded);

  var inputTypeToKeysetSpecMap = {};

  var numberKeysetSpec = {};
  numberKeysetSpec[keysetSpecNode.ID] = 'fi.compact.numberpad';
  numberKeysetSpec[keysetSpecNode.LAYOUT] = 'compactkbd-numberpad';
  numberKeysetSpec[keysetSpecNode.DATA] = i18n.input.chrome.inputview.content.
      compact.numberpad.keyNumberpadCharacters();
  inputTypeToKeysetSpecMap[ContextType.NUMBER] = numberKeysetSpec;

  var phoneKeysetSpec = {};
  phoneKeysetSpec[keysetSpecNode.ID] = 'fi.compact.phonepad';
  phoneKeysetSpec[keysetSpecNode.LAYOUT] = 'compactkbd-numberpad';
  phoneKeysetSpec[keysetSpecNode.DATA] = i18n.input.chrome.inputview.content.
      compact.numberpad.keyPhonepadCharacters();
  inputTypeToKeysetSpecMap[ContextType.PHONE] = phoneKeysetSpec;

  i18n.input.chrome.inputview.content.ContextlayoutUtil.generateContextLayouts(
      inputTypeToKeysetSpecMap, google.ime.chrome.inputview.onConfigLoaded);
}) ();
