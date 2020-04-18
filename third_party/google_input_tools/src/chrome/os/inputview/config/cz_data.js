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
  var viewIdPrefix_ = '102kbd-k-';

  var keyCharacters = [
    ['\u003b', '\u030a', '\u0060', '\u007e'], // TLDE
    ['\u002b', '\u0031', '\u0021', '\u0303'], // AE01
    ['\u011b', '\u0032', '\u0040', '\u030c'], // AE02
    ['\u0161', '\u0033', '\u0023', '\u0302'], // AE03
    ['\u010d', '\u0034', '\u0024', '\u0306'], // AE04
    ['\u0159', '\u0035', '\u0025', '\u030a'], // AE05
    ['\u017e', '\u0036', '\u005e', '\u0328'], // AE06
    ['\u00fd', '\u0037', '\u0026', '\u0300'], // AE07
    ['\u00e1', '\u0038', '\u002a', '\u0307'], // AE08
    ['\u00ed', '\u0039', '\u007b', '\u0301'], // AE09
    ['\u00e9', '\u0030', '\u007d', '\u030b'], // AE10
    ['\u003d', '\u0025', '\u005c', '\u0308'], // AE11
    ['\u0301', '\u030c', '\u0304', '\u0327'], // AE12
    ['\u0071', '\u0051', '\u005c', '\u03a9'], // AD01
    ['\u0077', '\u0057', '\u007c', '\u0141'], // AD02
    ['\u0065', '\u0045', '\u20ac', '\u0045'], // AD03
    ['\u0072', '\u0052', '\u00b6', '\u00ae'], // AD04
    ['\u0074', '\u0054', '\u0167', '\u0166'], // AD05
    ['\u007a', '\u005a', '\u2190', '\u00a5'], // AD06
    ['\u0075', '\u0055', '\u2193', '\u2191'], // AD07
    ['\u0069', '\u0049', '\u2192', '\u0131'], // AD08
    ['\u006f', '\u004f', '\u00f8', '\u00d8'], // AD09
    ['\u0070', '\u0050', '\u00fe', '\u00de'], // AD10
    ['\u00fa', '\u002f', '\u005b', '\u00f7'], // AD11
    ['\u0029', '\u0028', '\u005d', '\u00d7'], // AD12
    ['\u0061', '\u0041', '\u007e', '\u00c6'], // AC01
    ['\u0073', '\u0053', '\u0111', '\u00a7'], // AC02
    ['\u0064', '\u0044', '\u0110', '\u00d0'], // AC03
    ['\u0066', '\u0046', '\u005b', '\u00aa'], // AC04
    ['\u0067', '\u0047', '\u005d', '\u014a'], // AC05
    ['\u0068', '\u0048', '\u0060', '\u0126'], // AC06
    ['\u006a', '\u004a', '\u0027', '\u031b'], // AC07
    ['\u006b', '\u004b', '\u0142', '\u0026'], // AC08
    ['\u006c', '\u004c', '\u0141', '\u0141'], // AC09
    ['\u016f', '\u0022', '\u0024', '\u030b'], // AC10
    ['\u00a7', '\u0021', '\u0027', '\u00df'], // AC11
    ['\u0308', '\u0027', '\u005c', '\u007c'], // BKSL
    ['\u005c', '\u007c', '\u002f', '\u00a6'], // LSGT
    ['\u0079', '\u0059', '\u00b0', '\u003c'], // AB01
    ['\u0078', '\u0058', '\u0023', '\u003e'], // AB02
    ['\u0063', '\u0043', '\u0026', '\u00a9'], // AB03
    ['\u0076', '\u0056', '\u0040', '\u2018'], // AB04
    ['\u0062', '\u0042', '\u007b', '\u2019'], // AB05
    ['\u006e', '\u004e', '\u007d', '\u004e'], // AB06
    ['\u006d', '\u004d', '\u005e', '\u00ba'], // AB07
    ['\u002c', '\u003f', '\u003c', '\u00d7'], // AB08
    ['\u002e', '\u003a', '\u003e', '\u00f7'], // AB09
    ['\u002d', '\u005f', '\u002a', '\u0307'], // AB10
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
    0xBB, // AE11
    0xBF, // AE12
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
    0xDB, // AD11
    0xDD, // AD12
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
    0xDC, // BKSL
    0xE2, // LTGT
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
      keyCharacters, viewIdPrefix_, true, true, keyCodes);
  data['id'] = 'cz';
  google.ime.chrome.inputview.onConfigLoaded(data);
}) ();
