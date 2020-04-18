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
    ['\u0028', '\u0029', '\u005b', '\u005d'], // TLDE
    ['\u0031', '\u0021'], // AE01
    ['\u0032', '\u003f'], // AE02
    ['\u0033', '\u002b', '\u2020', '\u2020'], // AE03
    ['\u0034', '\u0022'], // AE04
    ['\u0035', '\u0025', '\u2329', '\u232a'], // AE05
    ['\u0036', '\u003d', '\u2014', '\u2014'], // AE06
    ['\u0037', '\u003a', '\u2026', '\u2026'], // AE07
    ['\u0038', '\u002f', '\u0300', '\u0301'], // AE08
    ['\u0039', '\u2013'], // AE09
    ['\u0030', '\u2116'], // AE10
    ['\u002d', '\u0024', '\u2011', '\u20ac'], // AE11
    ['\u002e', '\u20ac'], // AE12
    ['\u002c', '\u044b', '\u2019', '\u2018'], // AD01
    ['\u0443', '\u0423'], // AD02
    ['\u0435', '\u0415', '\u044d', '\u042d'], // AD03
    ['\u0438', '\u0418', '\u045d', '\u040d'], // AD04
    ['\u0448', '\u0428'], // AD05
    ['\u0449', '\u0429'], // AD06
    ['\u043a', '\u041a', '\u00a9', '\u00a9'], // AD07
    ['\u0441', '\u0421', '\u00a9', '\u00a9'], // AD08
    ['\u0434', '\u0414'], // AD09
    ['\u0437', '\u0417'], // AD10
    ['\u0446', '\u0426'], // AD11
    ['\u003b', '\u00a7'], // AD12
    ['\u044c', '\u045d', '\u044b', '\u042b'], // AC01
    ['\u044f', '\u042f', '\u0463', '\u0462'], // AC02
    ['\u0430', '\u0410'], // AC03
    ['\u043e', '\u041e'], // AC04
    ['\u0436', '\u0416'], // AC05
    ['\u0433', '\u0413'], // AC06
    ['\u0442', '\u0422', '\u2122', '\u2122'], // AC07
    ['\u043d', '\u041d'], // AC08
    ['\u0432', '\u0412'], // AC09
    ['\u043c', '\u041c'], // AC10
    ['\u0447', '\u0427'], // AC11
    ['\u201e', '\u201c', '\u00ab', '\u00bb'], // BKSL
    ['\u045d', '\u040d', '\u007c', '\u00a6'], // LSGT
    ['\u044e', '\u042e'], // AB01
    ['\u0439', '\u0419', '\u046d', '\u046c'], // AB02
    ['\u044a', '\u042a', '\u046b', '\u046a'], // AB03
    ['\u044d', '\u042d'], // AB04
    ['\u0444', '\u0424'], // AB05
    ['\u0445', '\u0425'], // AB06
    ['\u043f', '\u041f'], // AB07
    ['\u0440', '\u0420', '\u00ae', '\u00ae'], // AB08
    ['\u043b', '\u041b'], // AB09
    ['\u0431', '\u0411'], // AB10
    ['\u0020', '\u0020', '\u00a0', '\u00a0'] // SPCE
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
    0xBE, // AE12
    0xBC, // AD01
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
    0xDF, // AB08
    0x51, // AB09
    0xBF, // AB10
    0x20  // SPCE
  ];

  var data = i18n.input.chrome.inputview.content.util.createData(
      keyCharacters, viewIdPrefix_, true, true, keyCodes);
  data['id'] = 'bg';
  google.ime.chrome.inputview.onConfigLoaded(data);
}) ();
