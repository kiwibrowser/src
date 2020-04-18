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
    ['\u0060', '\u007e'], // TLDE
    ['\u0031', '\u0021', '\u0000', '\u00b9'], // AE01
    ['\u0032', '\u0040', '\u00bd', '\u00b2'], // AE02
    ['\u0033', '\u0023', '\u00a3', '\u00b3'], // AE03
    ['\u0034', '\u0024', '\u00bc', '\u00be'], // AE04
    ['\u0035', '\u0025', '\u20ac', '\u0000'], // AE05
    ['\u0036', '\u005e'], // AE06
    ['\u0037', '\u0026', '\u03f0', '\u0000'], // AE07
    ['\u0038', '\u002a'], // AE08
    ['\u0039', '\u0028'], // AE09
    ['\u0030', '\u0029', '\u00b0', '\u0000'], // AE10
    ['\u002d', '\u005f'], // AE11
    ['\u003d', '\u002b'], // AE12
    ['\u003b', '\u003a', '\u00b7', '\u0000'], // AD01
    ['\u03c2', '\u03a3', '\u03db', '\u03da'], // AD02
    ['\u03b5', '\u0395', '\u20ac', '\u0000'], // AD03
    ['\u03c1', '\u03a1', '\u00ae', '\u03f1'], // AD04
    ['\u03c4', '\u03a4'], // AD05
    ['\u03c5', '\u03a5'], // AD06
    ['\u03b8', '\u0398', '\u03d1', '\u03f4'], // AD07
    ['\u03b9', '\u0399', '\u037b', '\u03fd'], // AD08
    ['\u03bf', '\u039f'], // AD09
    ['\u03c0', '\u03a0', '\u03e1', '\u03e0'], // AD10
    ['\u005b', '\u007b', '\u0303', '\u0304'], // AD11
    ['\u005d', '\u007d', '\u0345', '\u0306'], // AD12
    ['\u005c', '\u007c'], // BKSL
    ['\u03b1', '\u0391'], // AC01
    ['\u03c3', '\u03a3'], // AC02
    ['\u03b4', '\u0394', '\u2193', '\u2191'], // AC03
    ['\u03c6', '\u03a6', '\u03d5', '\u0000'], // AC04
    ['\u03b3', '\u0393', '\u03dd', '\u03dc'], // AC05
    ['\u03b7', '\u0397'], // AC06
    ['\u03be', '\u039e', '\u037c', '\u03fe'], // AC07
    ['\u03ba', '\u039a', '\u03df', '\u03de'], // AC08
    ['\u03bb', '\u039b', '\u03f2', '\u03f9'], // AC09
    ['\u0301', '\u0308', '\u0301', '\u0000'], // AC10
    ['\u0027', '\u0022', '\u0300', '\u0000'], // AC11
    ['\u03b6', '\u0396', '\u037d', '\u03ff'], // AB01
    ['\u03c7', '\u03a7', '\u2192', '\u2190'], // AB02
    ['\u03c8', '\u03a8', '\u00a9', '\u0000'], // AB03
    ['\u03c9', '\u03a9', '\u03d6', '\u0000'], // AB04
    ['\u03b2', '\u0392', '\u03d0', '\u0000'], // AB05
    ['\u03bd', '\u039d', '\u0374', '\u0375'], // AB06
    ['\u03bc', '\u039c', '\u03fb', '\u03fa'], // AB07
    ['\u002c', '\u003c', '\u00ab', '\u0000'], // AB08
    ['\u002e', '\u003e', '\u00bb', '\u00b7'], // AB09
    ['\u002f', '\u003f'], // AB10
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
      keyCharacters, viewIdPrefix_, false, true, keyCodes);
  data['id'] = 'gr';
  google.ime.chrome.inputview.onConfigLoaded(data);
}) ();
