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
goog.require('i18n.input.chrome.inputview.content.util');

(function() {
  var viewIdPrefix_ = '101kbd-k-';

  var keyCharacters = [
    ['\u0024', '\u007e', '\u0303', '\u0000'], // TLDE
    ['\u0026', '\u0025'], // AE01
    ['\u005b', '\u0037', '\u00a4', '\u0000'], // AE02
    ['\u007b', '\u0035', '\u00a2', '\u0000'], // AE03
    ['\u007d', '\u0033', '\u00a5', '\u0000'], // AE04
    ['\u0028', '\u0031', '\u20ac', '\u0000'], // AE05
    ['\u003d', '\u0039', '\u00a3', '\u0000'], // AE06
    ['\u002a', '\u0030'], // AE07
    ['\u0029', '\u0032', '\u00bd', '\u0000'], // AE08
    ['\u002b', '\u0034'], // AE09
    ['\u005d', '\u0036'], // AE10
    ['\u0021', '\u0038', '\u00a1', '\u0000'], // AE11
    ['\u0023', '\u0060', '\u0300', '\u0000'], // AE12
    ['\u003b', '\u003a', '\u0308', '\u0000'], // AD01
    ['\u002c', '\u003c', '\u00ab', '\u0000'], // AD02
    ['\u002e', '\u003e', '\u00bb', '\u0000'], // AD03
    ['\u0070', '\u0050', '\u00b6', '\u00a7'], // AD04
    ['\u0079', '\u0059', '\u00fc', '\u00dc'], // AD05
    ['\u0066', '\u0046'], // AD06
    ['\u0067', '\u0047'], // AD07
    ['\u0063', '\u0043', '\u00e7', '\u00c7'], // AD08
    ['\u0072', '\u0052', '\u00ae', '\u2122'], // AD09
    ['\u006c', '\u004c'], // AD10
    ['\u002f', '\u003f', '\u00bf', '\u0000'], // AD11
    ['\u0040', '\u005e', '\u0302', '\u030c'], // AD12
    ['\u005c', '\u007c'], // BKSL
    ['\u0061', '\u0041', '\u00e5', '\u00c5'], // AC01
    ['\u006f', '\u004f', '\u00f8', '\u00d8'], // AC02
    ['\u0065', '\u0045', '\u00e6', '\u00c6'], // AC03
    ['\u0075', '\u0055', '\u00e9', '\u00c9'], // AC04
    ['\u0069', '\u0049'], // AC05
    ['\u0064', '\u0044', '\u00f0', '\u00d0'], // AC06
    ['\u0068', '\u0048', '\u0301', '\u0000'], // AC07
    ['\u0074', '\u0054', '\u00fe', '\u00de'], // AC08
    ['\u006e', '\u004e', '\u00f1', '\u00d1'], // AC09
    ['\u0073', '\u0053', '\u00df', '\u0000'], // AC10
    ['\u002d', '\u005f', '\u2010', '\u0000'], // AC11
    ['\u0027', '\u0022', '\u0301', '\u0000'], // AB01
    ['\u0071', '\u0051'], // AB02
    ['\u006a', '\u004a'], // AB03
    ['\u006b', '\u004b'], // AB04
    ['\u0078', '\u0058'], // AB05
    ['\u0062', '\u0042'], // AB06
    ['\u006d', '\u004d'], // AB07
    ['\u0077', '\u0057'], // AB08
    ['\u0076', '\u0056'], // AB09
    ['\u007a', '\u005a'], // AB10
    ['\u0020', '\u0020'] // SPCE
  ];

  var keyCodes = [
    0xC0, // TLDE
    0x31, // AE01
    0x32, // AE02
    0xDE, // AE03
    0xBF, // AE04
    0x35, // AE05
    0x36, // AE06
    0x37, // AE07
    0x38, // AE08
    0x39, // AE09
    0x30, // AE10
    0xBD, // AE11
    0xBB, // AE12
    0xBA, // AD01
    0xBC, // AD02
    0xBE, // AD03
    0x50, // AD04
    0x59, // AD05
    0x46, // AD06
    0x47, // AD07
    0x43, // AD08
    0x52, // AD09
    0x4C, // AD10
    0xBF, // AD11
    0xDD, // AD12
    0xDC, // BKSL
    0x41, // AC01
    0x4F, // AC02
    0x45, // AC03
    0x55, // AC04
    0x49, // AC05
    0x44, // AC06
    0x48, // AC07
    0x54, // AC08
    0x4E, // AC09
    0x53, // AC10
    0xBD, // AC11
    0x5A, // AB01
    0x51, // AB02
    0x4A, // AB03
    0x4B, // AB04
    0x58, // AB05
    0x42, // AB06
    0x4D, // AB07
    0x57, // AB08
    0x56, // AB09
    0x5A, // AB10
    0x20  // SPCE
  ];

  var data = i18n.input.chrome.inputview.content.util.createData(
      keyCharacters, viewIdPrefix_, false, true, keyCodes);
  data['id'] = 'us-dvp';
  google.ime.chrome.inputview.onConfigLoaded(data);
}) ();
