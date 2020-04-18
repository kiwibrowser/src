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
    ['\u044e', '\u042e'], // TLDE
    ['\u0031', '\u0021'], // AE01
    ['\u0032', '\u0040'], // AE02
    ['\u0033', '\u0451'], // AE03
    ['\u0034', '\u0401'], // AE04
    ['\u0035', '\u044a'], // AE05
    ['\u0036', '\u042a'], // AE06
    ['\u0037', '\u0026'], // AE07
    ['\u0038', '\u002a'], // AE08
    ['\u0039', '\u0028'], // AE09
    ['\u0030', '\u0029'], // AE10
    ['\u002d', '\u005f'], // AE11
    ['\u0447', '\u0427'], // AE12
    ['\u044f', '\u042f'], // AD01
    ['\u0432', '\u0412'], // AD02
    ['\u0435', '\u0415'], // AD03
    ['\u0440', '\u0420'], // AD04
    ['\u0442', '\u0422'], // AD05
    ['\u044b', '\u042b'], // AD06
    ['\u0443', '\u0423'], // AD07
    ['\u0438', '\u0418'], // AD08
    ['\u043e', '\u041e'], // AD09
    ['\u043f', '\u041f'], // AD10
    ['\u0448', '\u0428'], // AD11
    ['\u0449', '\u0429'], // AD12
    ['\u044d', '\u042d'], // BKSL
    ['\u0430', '\u0410'], // AC01
    ['\u0441', '\u0421'], // AC02
    ['\u0434', '\u0414'], // AC03
    ['\u0444', '\u0424'], // AC04
    ['\u0433', '\u0413'], // AC05
    ['\u0445', '\u0425'], // AC06
    ['\u0439', '\u0419'], // AC07
    ['\u043a', '\u041a'], // AC08
    ['\u043b', '\u041b'], // AC09
    ['\u003b', '\u003a'], // AC10
    ['\u0027', '\u0022'], // AC11
    ['\u0437', '\u0417'], // AB01
    ['\u044c', '\u042c'], // AB02
    ['\u0446', '\u0426'], // AB03
    ['\u0436', '\u0416'], // AB04
    ['\u0431', '\u0411'], // AB05
    ['\u043d', '\u041d'], // AB06
    ['\u043c', '\u041c'], // AB07
    ['\u002c', '\u003c'], // AB08
    ['\u002e', '\u003e'], // AB09
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
      keyCharacters, viewIdPrefix_, false, false, keyCodes);
  data['id'] = 'ru-phonetic';
  google.ime.chrome.inputview.onConfigLoaded(data);
}) ();
