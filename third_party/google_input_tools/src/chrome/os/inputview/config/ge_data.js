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
    ['\u201e', '\u201c', '\u201e', '\u007e'], // TLDE
    ['\u0031', '\u0021', '\u0027', '\u0000'], // AE01
    ['\u0032', '\u0040', '\u201e', '\u0000'], // AE02
    ['\u0033', '\u0023', '\u201c', '\u0000'], // AE03
    ['\u0034', '\u0024', '\u2116', '\u0000'], // AE04
    ['\u0035', '\u0025', '\u20ac', '\u0000'], // AE05
    ['\u0036', '\u005e'], // AE06
    ['\u0037', '\u0026', '\u00a7', '\u0000'], // AE07
    ['\u0038', '\u002a', '\u00b0', '\u0000'], // AE08
    ['\u0039', '\u0028'], // AE09
    ['\u0030', '\u0029'], // AE10
    ['\u002d', '\u005f', '\u2014', '\u0000'], // AE11
    ['\u003d', '\u002b', '\u2013', '\u0000'], // AE12
    ['\u10e5', '\u0051'], // AD01
    ['\u10ec', '\u10ed'], // AD02
    ['\u10d4', '\u0045', '\u10f1', '\u0000'], // AD03
    ['\u10e0', '\u10e6', '\u00ae', '\u0000'], // AD04
    ['\u10e2', '\u10d7'], // AD05
    ['\u10e7', '\u0059', '\u10f8', '\u0000'], // AD06
    ['\u10e3', '\u0055'], // AD07
    ['\u10d8', '\u0049', '\u10f2', '\u0000'], // AD08
    ['\u10dd', '\u004f'], // AD09
    ['\u10de', '\u0050'], // AD10
    ['\u005b', '\u007b'], // AD11
    ['\u005d', '\u007d'], // AD12
    ['\u005c', '\u007c', '\u007e', '\u007e'], // BKSL
    ['\u10d0', '\u0041', '\u10fa', '\u0000'], // AC01
    ['\u10e1', '\u10e8'], // AC02
    ['\u10d3', '\u0044'], // AC03
    ['\u10e4', '\u0046', '\u10f6', '\u0000'], // AC04
    ['\u10d2', '\u0047', '\u10f9', '\u0000'], // AC05
    ['\u10f0', '\u0048', '\u10f5', '\u0000'], // AC06
    ['\u10ef', '\u10df', '\u10f7', '\u0000'], // AC07
    ['\u10d9', '\u004b'], // AC08
    ['\u10da', '\u004c'], // AC09
    ['\u003b', '\u003a'], // AC10
    ['\u0027', '\u0022'], // AC11
    ['\u10d6', '\u10eb'], // AB01
    ['\u10ee', '\u0058', '\u10f4', '\u0000'], // AB02
    ['\u10ea', '\u10e9', '\u00a9', '\u0000'], // AB03
    ['\u10d5', '\u0056', '\u10f3', '\u0000'], // AB04
    ['\u10d1', '\u0042'], // AB05
    ['\u10dc', '\u004e', '\u10fc', '\u0000'], // AB06
    ['\u10db', '\u004d'], // AB07
    ['\u002c', '\u003c', '\u00ab', '\u0000'], // AB08
    ['\u002e', '\u003e', '\u00bb', '\u0000'], // AB09
    ['\u002f', '\u003f', '\u10fb', '\u0000'], // AB10
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
  data['id'] = 'ge';
  google.ime.chrome.inputview.onConfigLoaded(data);
}) ();
