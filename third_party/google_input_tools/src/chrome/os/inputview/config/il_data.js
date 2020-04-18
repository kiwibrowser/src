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
    ['\u003b', '\u007e', '\u05b0', '\u05b0'], // TLDE
    ['\u0031', '\u0021', '\u05b1', '\u05b1'], // AE01
    ['\u0032', '\u0040', '\u05b2', '\u05b2'], // AE02
    ['\u0033', '\u0023', '\u05b3', '\u05b3'], // AE03
    ['\u0034', '\u0024', '\u05b4', '\u05b4'], // AE04
    ['\u0035', '\u0025', '\u05b5', '\u05b5'], // AE05
    ['\u0036', '\u005e', '\u05b6', '\u05b6'], // AE06
    ['\u0037', '\u0026', '\u05b7', '\u05b7'], // AE07
    ['\u0038', '\u002a', '\u05b8', '\u05b8'], // AE08
    ['\u0039', '\u0029', '\u05c2', '\u05c2'], // AE09
    ['\u0030', '\u0028', '\u05c1', '\u05c1'], // AE10
    ['\u002d', '\u005f', '\u05b9', '\u05b9'], // AE11
    ['\u003d', '\u002b', '\u05bc', '\u05bc'], // AE12
    ['\u002f', '\u0051'], // AD01
    ['\u0027', '\u0057'], // AD02
    ['\u05e7', '\u0045', '\u20ac', '\u20ac'], // AD03
    ['\u05e8', '\u0052'], // AD04
    ['\u05d0', '\u0054'], // AD05
    ['\u05d8', '\u0059'], // AD06
    ['\u05d5', '\u0055'], // AD07
    ['\u05df', '\u0049'], // AD08
    ['\u05dd', '\u004f'], // AD09
    ['\u05e4', '\u0050'], // AD10
    ['\u005d', '\u007d', '\u05bf', '\u05bf'], // AD11
    ['\u005b', '\u007b', '\u05bd', '\u05bd'], // AD12
    ['\u005c', '\u007c', '\u05bb', '\u05bb'], // BKSL
    ['\u05e9', '\u0041', '\u20aa', '\u20aa'], // AC01
    ['\u05d3', '\u0053'], // AC02
    ['\u05d2', '\u0044'], // AC03
    ['\u05db', '\u0046'], // AC04
    ['\u05e2', '\u0047'], // AC05
    ['\u05d9', '\u0048'], // AC06
    ['\u05d7', '\u004a'], // AC07
    ['\u05dc', '\u004b'], // AC08
    ['\u05da', '\u004c'], // AC09
    ['\u05e3', '\u003a'], // AC10
    ['\u002c', '\u0022'], // AC11
    ['\u05d6', '\u005a'], // AB01
    ['\u05e1', '\u0058'], // AB02
    ['\u05d1', '\u0043'], // AB03
    ['\u05d4', '\u0056'], // AB04
    ['\u05e0', '\u0042'], // AB05
    ['\u05de', '\u004e'], // AB06
    ['\u05e6', '\u004d'], // AB07
    ['\u05ea', '\u003c'], // AB08
    ['\u05e5', '\u003e'], // AB09
    ['\u002e', '\u003f', '\u05c3', '\u05c3'], // AB10
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
  data['id'] = 'il';
  google.ime.chrome.inputview.onConfigLoaded(data);
}) ();
