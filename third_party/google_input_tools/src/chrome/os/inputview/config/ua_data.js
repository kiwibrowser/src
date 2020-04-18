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
    ['\u2019', '\u0027', '\u0301', '\u007e'], // TLDE
    ['\u0031', '\u0021'], // AE01
    ['\u0032', '\u0022', '\u00b2', '\u0000'], // AE02
    ['\u0033', '\u2116', '\u00a7', '\u20b4'], // AE03
    ['\u0034', '\u003b', '\u0024', '\u20ac'], // AE04
    ['\u0035', '\u0025', '\u00b0', '\u0000'], // AE05
    ['\u0036', '\u003a', '\u003c', '\u0000'], // AE06
    ['\u0037', '\u003f', '\u003e', '\u0000'], // AE07
    ['\u0038', '\u002a', '\u2022', '\u0000'], // AE08
    ['\u0039', '\u0028', '\u005b', '\u007b'], // AE09
    ['\u0030', '\u0029', '\u005d', '\u007d'], // AE10
    ['\u002d', '\u005f', '\u2014', '\u2013'], // AE11
    ['\u003d', '\u002b', '\u2260', '\u00b1'], // AE12
    ['\u0439', '\u0419'], // AD01
    ['\u0446', '\u0426'], // AD02
    ['\u0443', '\u0423', '\u045e', '\u040e'], // AD03
    ['\u043a', '\u041a', '\u00ae', '\u0000'], // AD04
    ['\u0435', '\u0415', '\u0451', '\u0401'], // AD05
    ['\u043d', '\u041d'], // AD06
    ['\u0433', '\u0413'], // AD07
    ['\u0448', '\u0428'], // AD08
    ['\u0449', '\u0429'], // AD09
    ['\u0437', '\u0417'], // AD10
    ['\u0445', '\u0425'], // AD11
    ['\u0457', '\u0407', '\u044a', '\u042a'], // AD12
    ['\u0444', '\u0424'], // AC01
    ['\u0456', '\u0406', '\u044b', '\u042b'], // AC02
    ['\u0432', '\u0412'], // AC03
    ['\u0430', '\u0410'], // AC04
    ['\u043f', '\u041f'], // AC05
    ['\u0440', '\u0420'], // AC06
    ['\u043e', '\u041e'], // AC07
    ['\u043b', '\u041b'], // AC08
    ['\u0434', '\u0414'], // AC09
    ['\u0436', '\u0416'], // AC10
    ['\u0454', '\u0404', '\u044d', '\u042d'], // AC11
    ['\u0491', '\u0490', '\u005c', '\u007c'], // BKSL
    ['\u002f', '\u007c', '\u007c', '\u00a6'], // LSGT
    ['\u044f', '\u042f'], // AB01
    ['\u0447', '\u0427'], // AB02
    ['\u0441', '\u0421', '\u00a9', '\u0000'], // AB03
    ['\u043c', '\u041c'], // AB04
    ['\u0438', '\u0418'], // AB05
    ['\u0442', '\u0422', '\u2122', '\u0000'], // AB06
    ['\u044c', '\u042c'], // AB07
    ['\u0431', '\u0411', '\u00ab', '\u201e'], // AB08
    ['\u044e', '\u042e', '\u00bb', '\u201c'], // AB09
    ['\u002e', '\u002c', '\u002f', '\u2026'], // AB10
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
  data['id'] = 'ua';
  google.ime.chrome.inputview.onConfigLoaded(data);
}) ();
