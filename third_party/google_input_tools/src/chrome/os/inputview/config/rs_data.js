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
    ['\u0060', '\u007e', '\u00b0', '\u00ac'], // TLDE
    ['\u0031', '\u0021', '\u0000', '\u0000'], // AE01
    ['\u0032', '\u0022', '\u0000', '\u0000'], // AE02
    ['\u0033', '\u0023', '\u0302', '\u0000'], // AE03
    ['\u0034', '\u0024', '\u0000', '\u0000'], // AE04
    ['\u0035', '\u0025', '\u0000', '\u0000'], // AE05
    ['\u0036', '\u0026', '\u0000', '\u0000'], // AE06
    ['\u0037', '\u002f', '\u0300', '\u0000'], // AE07
    ['\u0038', '\u0028', '\u0000', '\u0000'], // AE08
    ['\u0039', '\u0029', '\u0301', '\u0000'], // AE09
    ['\u0030', '\u003d', '\u0000', '\u0000'], // AE10
    ['\u0027', '\u003f', '\u0304', '\u0000'], // AE11
    ['\u002b', '\u002a', '\u0000', '\u0000'], // AE12
    ['\u0459', '\u0409', '\u005c', '\u0000'], // AD01
    ['\u045a', '\u040a', '\u007c', '\u0000'], // AD02
    ['\u0435', '\u0415', '\u20ac', '\u00a3'], // AD03
    ['\u0440', '\u0420', '\u00b6', '\u00ae'], // AD04
    ['\u0442', '\u0422', '\u2026', '\u0000'], // AD05
    ['\u0437', '\u0417', '\u2190', '\u00a5'], // AD06
    ['\u0443', '\u0423', '\u2193', '\u2191'], // AD07
    ['\u0438', '\u0418', '\u2192', '\u0000'], // AD08
    ['\u043e', '\u041e', '\u00a7', '\u0000'], // AD09
    ['\u043f', '\u041f', '\u0000', '\u0000'], // AD10
    ['\u0448', '\u0428', '\u00f7', '\u0000'], // AD11
    ['\u0452', '\u0402', '\u00d7', '\u0000'], // AD12
    ['\u0430', '\u0410', '\u0000', '\u0000'], // AC01
    ['\u0441', '\u0421', '\u201e', '\u00bb'], // AC02
    ['\u0434', '\u0414', '\u201c', '\u00ab'], // AC03
    ['\u0444', '\u0424', '\u005b', '\u0000'], // AC04
    ['\u0433', '\u0413', '\u005d', '\u0000'], // AC05
    ['\u0445', '\u0425', '\u0000', '\u0000'], // AC06
    ['\u0458', '\u0408', '\u0000', '\u0000'], // AC07
    ['\u043a', '\u041a', '\u0000', '\u0000'], // AC08
    ['\u043b', '\u041b', '\u0000', '\u0000'], // AC09
    ['\u0447', '\u0427', '\u0000', '\u0000'], // AC10
    ['\u045b', '\u040b', '\u0000', '\u0000'], // AC11
    ['\u0436', '\u0416', '\u00a4', '\u0000'], // BKSL
    ['\u003c', '\u003e', '\u007c', '\u00a6'], // LSGT
    ['\u0436', '\u0416', '\u2018', '\u0000'], // AB01
    ['\u045f', '\u040f', '\u2019', '\u0000'], // AB02
    ['\u0446', '\u0426', '\u00a2', '\u00a9'], // AB03
    ['\u0432', '\u0412', '\u0040', '\u0000'], // AB04
    ['\u0431', '\u0411', '\u007b', '\u0000'], // AB05
    ['\u043d', '\u041d', '\u007d', '\u0000'], // AB06
    ['\u043c', '\u041c', '\u005e', '\u0000'], // AB07
    ['\u002c', '\u003b', '\u003c', '\u0000'], // AB08
    ['\u002e', '\u003a', '\u003e', '\u0000'], // AB09
    ['\u002d', '\u005f', '\u2014', '\u2013'], // AB10
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
      keyCharacters, viewIdPrefix_, true, true, keyCodes);
  data['id'] = 'rs';
  google.ime.chrome.inputview.onConfigLoaded(data);
}) ();
