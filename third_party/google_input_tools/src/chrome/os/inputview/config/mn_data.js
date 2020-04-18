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
    ['\u003d', '\u002b', '\u00b0', '\u0000'], // TLDE
    ['\u0031', '\u2116', '\u00d7', '\u0000'], // AE01
    ['\u0032', '\u002d', '\u00f7', '\u0000'], // AE02
    ['\u0033', '\u0022', '\u00b1', '\u0000'], // AE03
    ['\u0034', '\u20ae', '\u00ac', '\u0000'], // AE04
    ['\u0035', '\u003a'], // AE05
    ['\u0036', '\u002e', '\u2260', '\u0000'], // AE06
    ['\u0037', '\u005f', '\u0026', '\u0000'], // AE07
    ['\u0038', '\u002c', '\u002a', '\u0000'], // AE08
    ['\u0039', '\u0025', '\u005b', '\u0000'], // AE09
    ['\u0030', '\u003f', '\u005d', '\u0000'], // AE10
    ['\u0435', '\u0415', '\u0058', '\u0000'], // AE11
    ['\u0449', '\u0429', '\u004c', '\u0000'], // AE12
    ['\u0444', '\u0424', '\u0027', '\u0000'], // AD01
    ['\u0446', '\u0426', '\u0060', '\u0000'], // AD02
    ['\u0443', '\u0423', '\u20ac', '\u0000'], // AD03
    ['\u0436', '\u0416', '\u00ae', '\u0000'], // AD04
    ['\u044d', '\u042d', '\u2122', '\u0000'], // AD05
    ['\u043d', '\u041d', '\u00a5', '\u0000'], // AD06
    ['\u0433', '\u0413', '\u201e', '\u0000'], // AD07
    ['\u0448', '\u0428', '\u201c', '\u0000'], // AD08
    ['\u04af', '\u04ae', '\u201d', '\u0000'], // AD09
    ['\u0437', '\u0417'], // AD10
    ['\u043a', '\u041a', '\u007b', '\u0000'], // AD11
    ['\u044a', '\u042a', '\u007d', '\u0000'], // AD12
    ['\u0021', '\u007c', '\u007c', '\u0000'], // BKSL
    ['\u0439', '\u0419', '\u00b5', '\u0000'], // AC01
    ['\u044b', '\u042b', '\u00a3', '\u0000'], // AC02
    ['\u0431', '\u0411', '\u0024', '\u0000'], // AC03
    ['\u04e9', '\u04e8', '\u201d', '\u0000'], // AC04
    ['\u0430', '\u0410', '\u044b', '\u0000'], // AC05
    ['\u0445', '\u0425', '\u042b', '\u0000'], // AC06
    ['\u0440', '\u0420', '\u044d', '\u0000'], // AC07
    ['\u043e', '\u041e', '\u042d', '\u0000'], // AC08
    ['\u043b', '\u041b', '\u2116', '\u0000'], // AC09
    ['\u0434', '\u0414', '\u00a7', '\u0000'], // AC10
    ['\u043f', '\u041f', '\u2026', '\u0000'], // AC11
    ['\u044f', '\u042f', '\u2014', '\u0000'], // AB01
    ['\u0447', '\u0427', '\u2013', '\u0000'], // AB02
    ['\u0451', '\u0401', '\u00a9', '\u0000'], // AB03
    ['\u0441', '\u0421'], // AB04
    ['\u043c', '\u041c'], // AB05
    ['\u0438', '\u0418', '\u003c', '\u0000'], // AB06
    ['\u0442', '\u0422', '\u003e', '\u0000'], // AB07
    ['\u044c', '\u042c', '\u00ab', '\u0000'], // AB08
    ['\u0432', '\u0412', '\u00bb', '\u0000'], // AB09
    ['\u044e', '\u042e', '\u005c', '\u0000'], // AB10
    ['\u0020', '\u0020', '\u00a0', '\u0000'] // SPCE
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
  data['id'] = 'mn';
  google.ime.chrome.inputview.onConfigLoaded(data);
}) ();
