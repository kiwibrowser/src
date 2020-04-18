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
    ['\u0022', '\u00e9', '\u003c', '\u00b0'], // TLDE
    ['\u0031', '\u0021', '\u003e', '\u00a1'], // AE01
    ['\u0032', '\u0027', '\u00a3', '\u00b2'], // AE02
    ['\u0033', '\u005e', '\u0023', '\u00b3'], // AE03
    ['\u0034', '\u002b', '\u0024', '\u00bc'], // AE04
    ['\u0035', '\u0025', '\u00bd', '\u215c'], // AE05
    ['\u0036', '\u0026', '\u00be', '\u0000'], // AE06
    ['\u0037', '\u002f', '\u007b', '\u0000'], // AE07
    ['\u0038', '\u0028', '\u005b', '\u0000'], // AE08
    ['\u0039', '\u0029', '\u005d', '\u00b1'], // AE09
    ['\u0030', '\u003d', '\u007d', '\u00b0'], // AE10
    ['\u002a', '\u003f', '\u005c', '\u00bf'], // AE11
    ['\u002d', '\u005f', '\u007c', '\u0000'], // AE12
    ['\u0071', '\u0051', '\u0040', '\u03a9'], // AD01
    ['\u0077', '\u0057', '\u0000', '\u0000'], // AD02
    ['\u0065', '\u0045', '\u20ac', '\u0000'], // AD03
    ['\u0072', '\u0052', '\u00b6', '\u00ae'], // AD04
    ['\u0074', '\u0054', '\u20ba', '\u0000'], // AD05
    ['\u0079', '\u0059', '\u2190', '\u00a5'], // AD06
    ['\u0075', '\u0055', '\u00fb', '\u00db'], // AD07
    ['\u0131', '\u0049', '\u00ee', '\u00ce'], // AD08
    ['\u006f', '\u004f', '\u00f4', '\u00d4'], // AD09
    ['\u0070', '\u0050', '\u0000', '\u0000'], // AD10
    ['\u011f', '\u011e', '\u0308', '\u030a'], // AD11
    ['\u00fc', '\u00dc', '\u007e', '\u0304'], // AD12
    ['\u002c', '\u003b', '\u0060', '\u0300'], // BKSL
    ['\u0061', '\u0041', '\u00e2', '\u00c2'], // AC01
    ['\u0073', '\u0053', '\u00a7', '\u0000'], // AC02
    ['\u0064', '\u0044', '\u0000', '\u0000'], // AC03
    ['\u0066', '\u0046', '\u00aa', '\u0000'], // AC04
    ['\u0067', '\u0047', '\u0000', '\u0000'], // AC05
    ['\u0068', '\u0048', '\u0000', '\u0000'], // AC06
    ['\u006a', '\u004a', '\u0309', '\u031b'], // AC07
    ['\u006b', '\u004b', '\u0000', '\u0000'], // AC08
    ['\u006c', '\u004c', '\u0000', '\u0000'], // AC09
    ['\u015f', '\u015e', '\u00b4', '\u0301'], // AC10
    ['\u0069', '\u0130', '\u0027', '\u030c'], // AC11
    ['\u007a', '\u005a', '\u00ab', '\u003c'], // AB01
    ['\u0078', '\u0058', '\u00bb', '\u003e'], // AB02
    ['\u0063', '\u0043', '\u00a2', '\u00a9'], // AB03
    ['\u0076', '\u0056', '\u201c', '\u2018'], // AB04
    ['\u0062', '\u0042', '\u201d', '\u2019'], // AB05
    ['\u006e', '\u004e'], // AB06
    ['\u006d', '\u004d', '\u00b5', '\u00ba'], // AB07
    ['\u00f6', '\u00d6', '\u00d7', '\u0000'], // AB08
    ['\u00e7', '\u00c7', '\u00b7', '\u00f7'], // AB09
    ['\u002e', '\u003a', '\u0307', '\u0307'], // AB10
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
    0xDF, // AE11
    0xBD, // AE12
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
    0xBC, // BKSL
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
    0xBF, // AB08
    0xDC, // AB09
    0xBE, // AB10
    0x20  // SPCE
  ];

  var data = i18n.input.chrome.inputview.content.util.createData(
      keyCharacters, viewIdPrefix_, false, true, keyCodes);
  data['id'] = 'tr';
  google.ime.chrome.inputview.onConfigLoaded(data);
}) ();
