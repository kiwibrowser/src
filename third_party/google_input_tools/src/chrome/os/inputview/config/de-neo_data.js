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
goog.require('i18n.input.chrome.inputview.content.util');

(function() {
  var viewIdPrefix_ = '101kbd-k-';

  var keyCharacters = [
    ['\u0302', '\u030c', '\u21bb', '\u02de'], // TLDE
    ['\u0031', '\u00b0', '\u00b9', '\u2081'], // AE01
    ['\u0032', '\u00a7', '\u00b2', '\u2082'], // AE02
    ['\u0033', '\u2113', '\u00b3', '\u2083'], // AE03
    ['\u0034', '\u00bb', '\u203a', '\u2640'], // AE04
    ['\u0035', '\u00ab', '\u2039', '\u2642'], // AE05
    ['\u0036', '\u0024', '\u00a2', '\u26a5'], // AE06
    ['\u0037', '\u20ac', '\u00a5', '\u03f0'], // AE07
    ['\u0038', '\u201e', '\u201a', '\u27e8'], // AE08
    ['\u0039', '\u201c', '\u2018', '\u27e9'], // AE09
    ['\u0030', '\u201d', '\u2019', '\u2080'], // AE10
    ['\u002d', '\u2014', '\u0000', '\u2011'], // AE11
    ['\u0300', '\u0327', '\u030a', '\u0000'], // AE12
    ['\u0078', '\u0058', '\u2026', '\u03be'], // AD01
    ['\u0076', '\u0056', '\u005f', '\u0000'], // AD02
    ['\u006c', '\u004c', '\u005b', '\u03bb'], // AD03
    ['\u0063', '\u0043', '\u005d', '\u03c7'], // AD04
    ['\u0077', '\u0057', '\u005e', '\u03c9'], // AD05
    ['\u006b', '\u004b', '\u0021', '\u03ba'], // AD06
    ['\u0068', '\u0048', '\u003c', '\u03c8'], // AD07
    ['\u0067', '\u0047', '\u003e', '\u03b3'], // AD08
    ['\u0066', '\u0046', '\u003d', '\u03c6'], // AD09
    ['\u0071', '\u0051', '\u0026', '\u03d5'], // AD10
    ['\u00df', '\u1e9e', '\u017f', '\u03c2'], // AD11
    ['\u0301', '\u0303', '\u0000', '\u0000'], // AD12
    ['\u005c', '\u007c'], // BKSL
    ['\u0075', '\u0055', '\u005c', '\u0000'], // AC01
    ['\u0069', '\u0049', '\u002f', '\u03b9'], // AC02
    ['\u0061', '\u0041', '\u007b', '\u03b1'], // AC03
    ['\u0065', '\u0045', '\u007d', '\u03b5'], // AC04
    ['\u006f', '\u004f', '\u002a', '\u03bf'], // AC05
    ['\u0073', '\u0053', '\u003f', '\u03c3'], // AC06
    ['\u006e', '\u004e', '\u0028', '\u03bd'], // AC07
    ['\u0072', '\u0052', '\u0029', '\u03c1'], // AC08
    ['\u0074', '\u0054', '\u002d', '\u03c4'], // AC09
    ['\u0064', '\u0044', '\u003a', '\u03b4'], // AC10
    ['\u0079', '\u0059', '\u0040', '\u03c5'], // AC11
    ['\u00fc', '\u00dc', '\u0023', '\u0000'], // AB01
    ['\u00f6', '\u00d6', '\u0024', '\u03f5'], // AB02
    ['\u00e4', '\u00c4', '\u007c', '\u03b7'], // AB03
    ['\u0070', '\u0050', '\u007e', '\u03c0'], // AB04
    ['\u007a', '\u005a', '\u0060', '\u03b6'], // AB05
    ['\u0062', '\u0042', '\u002b', '\u03b2'], // AB06
    ['\u006d', '\u004d', '\u0025', '\u03bc'], // AB07
    ['\u002c', '\u2013', '\u0022', '\u03f1'], // AB08
    ['\u002e', '\u2022', '\u0027', '\u03d1'], // AB09
    ['\u006a', '\u004a', '\u003b', '\u03b8'], // AB10
    ['\u0020', '\u0020', '\u0020', '\u00a0'] // SPCE
  ]; // WARNING: de-neo layout is 102 keyboard, but key count is 47!

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
    0xDB, // AE11
    0xDD, // AE12
    0x51, // AD01
    0x57, // AD02
    0x45, // AD03
    0x52, // AD04
    0x54, // AD05
    0x5A, // AD06
    0x55, // AD07
    0x49, // AD08
    0x4F, // AD09
    0x50, // AD10
    0xBA, // AD11
    0xBB, // AD12
    0xBF, // BKSL
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
    0x59, // AB01
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
      keyCharacters, viewIdPrefix_, false, true, keyCodes);
  data['id'] = 'de-neo';
  google.ime.chrome.inputview.onConfigLoaded(data);
}) ();
