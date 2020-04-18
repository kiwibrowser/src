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
    ['\u0060', '\u007e', '\u0300', '\u0303'], // TLDE
    ['\u0031', '\u0021'], // AE01
    ['\u0032', '\u0040'], // AE02
    ['\u0033', '\u0023'], // AE03
    ['\u0034', '\u0024'], // AE04
    ['\u0035', '\u0025'], // AE05
    ['\u0036', '\u005e', '\u0302', '\u0302'], // AE06
    ['\u0037', '\u0026'], // AE07
    ['\u0038', '\u002a'], // AE08
    ['\u0039', '\u0028', '\u0300', '\u0000'], // AE09
    ['\u0030', '\u0029'], // AE10
    ['\u005b', '\u007b'], // AE11
    ['\u005d', '\u007d', '\u0303', '\u0000'], // AE12
    ['\u0027', '\u0022', '\u0301', '\u0308'], // AD01
    ['\u002c', '\u003c', '\u0327', '\u030c'], // AD02
    ['\u002e', '\u003e', '\u0307', '\u00b7'], // AD03
    ['\u0070', '\u0050'], // AD04
    ['\u0079', '\u0059'], // AD05
    ['\u0066', '\u0046'], // AD06
    ['\u0067', '\u0047'], // AD07
    ['\u0063', '\u0043'], // AD08
    ['\u0072', '\u0052'], // AD09
    ['\u006c', '\u004c'], // AD10
    ['\u002f', '\u003f'], // AD11
    ['\u003d', '\u002b'], // AD12
    ['\u005c', '\u007c'], // BKSL
    ['\u0061', '\u0041'], // AC01
    ['\u006f', '\u004f'], // AC02
    ['\u0065', '\u0045'], // AC03
    ['\u0075', '\u0055'], // AC04
    ['\u0069', '\u0049'], // AC05
    ['\u0064', '\u0044'], // AC06
    ['\u0068', '\u0048'], // AC07
    ['\u0074', '\u0054'], // AC08
    ['\u006e', '\u004e'], // AC09
    ['\u0073', '\u0053'], // AC10
    ['\u002d', '\u005f'], // AC11
    ['\u003b', '\u003a', '\u0328', '\u030b'], // AB01
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
    0xDE, // AD01
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
    0xBB, // AD12
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
    0xBA, // AB01
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
  data['id'] = 'us-dvorak';
  google.ime.chrome.inputview.onConfigLoaded(data);
}) ();
