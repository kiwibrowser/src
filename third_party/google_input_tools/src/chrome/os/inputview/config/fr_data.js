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
    [ '\u00b2', '\u00b3', '\u00b9', '\u0327' ], // TLDE
    [ '\u0026', '\u0031', '\u030c', '\u0328' ], // AE01
    [ '\u00e9', '\u0032', '\u007e', '\u00c9' ], // AE02
    [ '\u0022', '\u0033', '\u0023', '\u0306' ], // AE03
    [ '\u0027', '\u0034', '\u007b', '\u2014' ], // AE04
    [ '\u0028', '\u0035', '\u005b', '\u2013' ], // AE05
    [ '\u002d', '\u0036', '\u007c', '\u2011' ], // AE06
    [ '\u00e8', '\u0037', '\u0060', '\u00c8' ], // AE07
    [ '\u005f', '\u0038', '\u005c', '\u2122' ], // AE08
    [ '\u00e7', '\u0039', '\u005e', '\u00c7' ], // AE09
    [ '\u00e0', '\u0030', '\u0040', '\u00c0' ], // AE10
    [ '\u0029', '\u00b0', '\u005d', '\u2260' ], // AE11
    [ '\u003d', '\u002b', '\u007d', '\u00b1' ], // AE12
    [ '\u0061', '\u0041', '\u00e6', '\u00c6' ], // AD01
    [ '\u007a', '\u005a', '\u00e2', '\u00c2' ], // AD02
    [ '\u0065', '\u0045', '\u20ac', '\u00a2' ], // AD03
    [ '\u0072', '\u0052', '\u00ea', '\u00ca' ], // AD04
    [ '\u0074', '\u0054', '\u00fe', '\u00de' ], // AD05
    [ '\u0079', '\u0059', '\u00ff', '\u0178' ], // AD06
    [ '\u0075', '\u0055', '\u00fb', '\u00db' ], // AD07
    [ '\u0069', '\u0049', '\u00ee', '\u00ce' ], // AD08
    [ '\u006f', '\u004f', '\u0153', '\u0152' ], // AD09
    [ '\u0070', '\u0050', '\u00f4', '\u00d4' ], // AD10
    [ '\u0302', '\u0308', '\u0303', '\u030a' ], // AD11
    [ '\u0024', '\u00a3', '\u00f8', '\u00d8' ], // AD12
    [ '\u0071', '\u0051', '\u00e4', '\u00c4' ], // AC01
    [ '\u0073', '\u0053', '\u00df', '\u201e' ], // AC02
    [ '\u0064', '\u0044', '\u00eb', '\u00cb' ], // AC03
    [ '\u0066', '\u0046', '\u2018', '\u201a' ], // AC04
    [ '\u0067', '\u0047', '\u2019', '\u00a5' ], // AC05
    [ '\u0068', '\u0048', '\u00f0', '\u00d0' ], // AC06
    [ '\u006a', '\u004a', '\u00fc', '\u00dc' ], // AC07
    [ '\u006b', '\u004b', '\u00ef', '\u00cf' ], // AC08
    [ '\u006c', '\u004c', '\u0140', '\u013f' ], // AC09
    [ '\u006d', '\u004d', '\u00f6', '\u00d6' ], // AC10
    [ '\u00f9', '\u0025', '\u0301', '\u00d9' ], // AC11
    [ '\u002a', '\u00b5', '\u0300', '\u0304' ], // BKSL
    [ '\u003c', '\u003e', '\u2264', '\u2265' ], // LSGT
    [ '\u0077', '\u0057', '\u00ab', '\u201c' ], // AB01
    [ '\u0078', '\u0058', '\u00bb', '\u201d' ], // AB02
    [ '\u0063', '\u0043', '\u00a9', '\u00ae' ], // AB03
    [ '\u0076', '\u0056', '\u202f', '\u2190' ], // AB04
    [ '\u0062', '\u0042', '\u2193', '\u2191' ], // AB05
    [ '\u006e', '\u004e', '\u00ac', '\u2192' ], // AB06
    [ '\u002c', '\u003f', '\u00bf', '\u2026' ], // AB07
    [ '\u003b', '\u002e', '\u00d7', '\u22c5' ], // AB08
    [ '\u003a', '\u002f', '\u00f7', '\u2215' ], // AB09
    [ '\u0021', '\u00a7', '\u00a1', '\u2212' ], // AB10
    [ '\u0020', '\u0020', '\u0020', '\u00a0' ] // SPCE
  ];

  var keyCodes = [
    0xDE, // TLDE
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
    0xDB, // AE11
    0xBB, // AE12
    0x41, // AD01
    0x5A, // AD02
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
    0x51, // AC01
    0x53, // AC02
    0x44, // AC03
    0x46, // AC04
    0x47, // AC05
    0x48, // AC06
    0x4A, // AC07
    0x4B, // AC08
    0x4C, // AC09
    0x4D, // AC10
    0xC0, // AC11
    0xDC, // BKSL
    0xE2, // LTGT
    0x57, // AB01
    0x58, // AB02
    0x43, // AB03
    0x56, // AB04
    0x42, // AB05
    0x4E, // AB06
    0xBC, // AB07
    0xBE, // AB08
    0xBF, // AB09
    0xDF, // AB10
    0x20  // SPCE
  ];

  var data = i18n.input.chrome.inputview.content.util.createData(
      keyCharacters, viewIdPrefix_, true, true, keyCodes, 'fr.compact.qwerty');
  data['id'] = 'fr';
  google.ime.chrome.inputview.onConfigLoaded(data);

  var keysetSpecNode =
      i18n.input.chrome.inputview.content.compact.util.CompactKeysetSpec;
  var letterKeysetSpec = {};
  letterKeysetSpec[keysetSpecNode.ID] = 'fr.compact.qwerty';
  letterKeysetSpec[keysetSpecNode.LAYOUT] = 'compactkbd-azerty';
  letterKeysetSpec[keysetSpecNode.DATA] =
      i18n.input.chrome.inputview.content.compact.letter.keyAzertyCharacters();

  var symbolKeysetSpec = {};
  symbolKeysetSpec[keysetSpecNode.ID] = 'fr.compact.symbol';
  symbolKeysetSpec[keysetSpecNode.LAYOUT] = 'compactkbd-qwerty';
  symbolKeysetSpec[keysetSpecNode.DATA] = i18n.input.chrome.inputview.content.
      compact.symbol.keyEUSymbolCharacters();

  var moreKeysetSpec = {};
  moreKeysetSpec[keysetSpecNode.ID] = 'fr.compact.more';
  moreKeysetSpec[keysetSpecNode.LAYOUT] = 'compactkbd-qwerty';
  moreKeysetSpec[keysetSpecNode.DATA] =
      i18n.input.chrome.inputview.content.compact.more.keyEUMoreCharacters();

  i18n.input.chrome.inputview.content.compact.util.generateCompactKeyboard(
      letterKeysetSpec, symbolKeysetSpec, moreKeysetSpec,
      google.ime.chrome.inputview.onConfigLoaded);

  var inputTypeToKeysetSpecMap = {};

  var numberKeysetSpec = {};
  numberKeysetSpec[keysetSpecNode.ID] = 'fr.compact.numberpad';
  numberKeysetSpec[keysetSpecNode.LAYOUT] = 'compactkbd-numberpad';
  numberKeysetSpec[keysetSpecNode.DATA] = i18n.input.chrome.inputview.content.
      compact.numberpad.keyNumberpadCharacters();
  inputTypeToKeysetSpecMap[ContextType.NUMBER] = numberKeysetSpec;

  var phoneKeysetSpec = {};
  phoneKeysetSpec[keysetSpecNode.ID] = 'fr.compact.phonepad';
  phoneKeysetSpec[keysetSpecNode.LAYOUT] = 'compactkbd-numberpad';
  phoneKeysetSpec[keysetSpecNode.DATA] = i18n.input.chrome.inputview.content.
      compact.numberpad.keyPhonepadCharacters();
  inputTypeToKeysetSpecMap[ContextType.PHONE] = phoneKeysetSpec;

  i18n.input.chrome.inputview.content.ContextlayoutUtil.generateContextLayouts(
      inputTypeToKeysetSpecMap, google.ime.chrome.inputview.onConfigLoaded);
}) ();
