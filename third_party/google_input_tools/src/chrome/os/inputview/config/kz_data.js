// Copyright 2016 The ChromeOS IME Authors. All Rights Reserved.
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
    ['\u0028', '\u0029'], // TLDE
    ['\u201e', '\u0021'], // AE01
    ['\u04d9', '\u04d8'], // AE02
    ['\u0456', '\u0406'], // AE03
    ['\u04a3', '\u04a2'], // AE04
    ['\u0493', '\u0492'], // AE05
    ['\u002c', '\u003b'], // AE06
    ['\u002e', '\u003a'], // AE07
    ['\u04af', '\u04ae'], // AE08
    ['\u04b1', '\u04b0'], // AE09
    ['\u049b', '\u049a'], // AE10
    ['\u04e9', '\u04e8'], // AE11
    ['\u04bb', '\u04ba'], // AE12
    ['\u0439', '\u0419'], // AD01
    ['\u0446', '\u0426'], // AD02
    ['\u0443', '\u0423'], // AD03
    ['\u043a', '\u041a'], // AD04
    ['\u0435', '\u0415'], // AD05
    ['\u043d', '\u041d'], // AD06
    ['\u0433', '\u0413'], // AD07
    ['\u0448', '\u0428'], // AD08
    ['\u0449', '\u0429'], // AD09
    ['\u0437', '\u0417'], // AD10
    ['\u0445', '\u0425'], // AD11
    ['\u044a', '\u042a'], // AD12
    ['\u005c', '\u002f'], // BKSL
    ['\u0444', '\u0424'], // AC01
    ['\u044b', '\u042b'], // AC02
    ['\u0432', '\u0412'], // AC03
    ['\u0430', '\u0410'], // AC04
    ['\u043f', '\u041f'], // AC05
    ['\u0440', '\u0420'], // AC06
    ['\u043e', '\u041e'], // AC07
    ['\u043b', '\u041b'], // AC08
    ['\u0434', '\u0414'], // AC09
    ['\u0436', '\u0416'], // AC10
    ['\u044d', '\u042d'], // AC11
    ['\u044f', '\u042f'], // AB01
    ['\u0447', '\u0427'], // AB02
    ['\u0441', '\u0421'], // AB03
    ['\u043c', '\u041c'], // AB04
    ['\u0438', '\u0418'], // AB05
    ['\u0442', '\u0422'], // AB06
    ['\u044c', '\u042c'], // AB07
    ['\u0431', '\u0411'], // AB08
    ['\u044e', '\u042e'], // AB09
    ['\u2116', '\u003f'], // AB10
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
  data['id'] = 'kz';
  google.ime.chrome.inputview.onConfigLoaded(data);
}) ();
