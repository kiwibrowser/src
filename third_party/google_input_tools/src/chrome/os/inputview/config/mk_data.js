// Copyright 2015 The ChromeOS IME Authors. All Rights Reserved.
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
    ['\u0300', '\u007e'], // TLDE
    ['\u0031', '\u0021'], // AE01
    ['\u0032', '\u201e'], // AE02
    ['\u0033', '\u201c'], // AE03
    ['\u0034', '\u0024'], // AE04
    ['\u0035', '\u0025'], // AE05
    ['\u0036', '\u005e'], // AE06
    ['\u0037', '\u0026'], // AE07
    ['\u0038', '\u002a'], // AE08
    ['\u0039', '\u0028'], // AE09
    ['\u0030', '\u0029'], // AE10
    ['\u002d', '\u005f'], // AE11
    ['\u003d', '\u002b'], // AE12
    ['\u0459', '\u0409'], // AD01
    ['\u045a', '\u040a'], // AD02
    ['\u0435', '\u0415'], // AD03
    ['\u0440', '\u0420'], // AD04
    ['\u0442', '\u0422'], // AD05
    ['\u0455', '\u0405'], // AD06
    ['\u0443', '\u0423'], // AD07
    ['\u0438', '\u0418'], // AD08
    ['\u043e', '\u041e'], // AD09
    ['\u043f', '\u041f'], // AD10
    ['\u0448', '\u0428'], // AD11
    ['\u0453', '\u0403'], // AD12
    ['\u0436', '\u0416'], // BKSL
    ['\u0430', '\u0410'], // AC01
    ['\u0441', '\u0421'], // AC02
    ['\u0434', '\u0414'], // AC03
    ['\u0444', '\u0424'], // AC04
    ['\u0433', '\u0413'], // AC05
    ['\u0445', '\u0425'], // AC06
    ['\u0458', '\u0408'], // AC07
    ['\u043a', '\u041a'], // AC08
    ['\u043b', '\u041b'], // AC09
    ['\u0447', '\u0427'], // AC10
    ['\u045c', '\u040c'], // AC11
    ['\u0437', '\u0417'], // AB01
    ['\u045f', '\u040f'], // AB02
    ['\u0446', '\u0426'], // AB03
    ['\u0432', '\u0412'], // AB04
    ['\u0431', '\u0411'], // AB05
    ['\u043d', '\u041d'], // AB06
    ['\u043c', '\u041c'], // AB07
    ['\u002c', '\u003b'], // AB08
    ['\u002e', '\u003a'], // AB09
    ['\u002f', '\u003f'], // AB10
    ['\u0020', '\u0020'] // SPCE
  ];

  var data = i18n.input.chrome.inputview.content.util.createData(
      keyCharacters, viewIdPrefix_, false, false);
  data['id'] = 'mk';
  google.ime.chrome.inputview.onConfigLoaded(data);
}) ();
