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
  var viewIdPrefix_ = '102kbd-k-';

  var keyCharacters = [
    ['\u002a', '\u002b', '\u00ac', '\u0000'], // TLDE
    ['\u0031', '\u0021', '\u0000', '\u0000'], // AE01
    ['\u0032', '\u0022', '\u00b2', '\u0000'], // AE02
    ['\u0033', '\u005e', '\u0023', '\u0000'], // AE03
    ['\u0034', '\u0024', '\u00bc', '\u0000'], // AE04
    ['\u0035', '\u0025', '\u00bd', '\u0000'], // AE05
    ['\u0036', '\u0026', '\u00be', '\u0000'], // AE06
    ['\u0037', '\u0027', '\u007b', '\u0000'], // AE07
    ['\u0038', '\u0028', '\u005b', '\u0000'], // AE08
    ['\u0039', '\u0029', '\u005d', '\u0000'], // AE09
    ['\u0030', '\u003d', '\u007d', '\u0000'], // AE10
    ['\u002f', '\u003f', '\u005c', '\u0000'], // AE11
    ['\u002d', '\u005f', '\u007c', '\u0000'], // AE12
    ['\u0066', '\u0046', '\u0040', '\u0000'], // AD01
    ['\u0067', '\u0047', '\u0000', '\u0000'], // AD02
    ['\u011f', '\u011e', '\u0000', '\u0000'], // AD03
    ['\u0131', '\u0049', '\u00b6', '\u0000'], // AD04
    ['\u006f', '\u004f', '\u0000', '\u0000'], // AD05
    ['\u0064', '\u0044', '\u00a5', '\u0000'], // AD06
    ['\u0072', '\u0052', '\u0000', '\u0000'], // AD07
    ['\u006e', '\u004e', '\u0000', '\u0000'], // AD08
    ['\u0068', '\u0048', '\u00f8', '\u0000'], // AD09
    ['\u0070', '\u0050', '\u00a3', '\u0000'], // AD10
    ['\u0071', '\u0051', '\u00a8', '\u0000'], // AD11
    ['\u0077', '\u0057', '\u007e', '\u0000'], // AD12
    ['\u0075', '\u0055', '\u00e6', '\u0000'], // AC01
    ['\u0069', '\u0130', '\u00a7', '\u0000'], // AC02
    ['\u0065', '\u0045', '\u20ac', '\u0000'], // AC03
    ['\u0061', '\u0041', '\u0000', '\u0000'], // AC04
    ['\u00fc', '\u00dc', '\u0000', '\u0000'], // AC05
    ['\u0074', '\u0054', '\u20ba', '\u0000'], // AC06
    ['\u006b', '\u004b', '\u0000', '\u0000'], // AC07
    ['\u006d', '\u004d', '\u0000', '\u0000'], // AC08
    ['\u006c', '\u004c', '\u0000', '\u0000'], // AC09
    ['\u0079', '\u0059', '\u0000', '\u0000'], // AC10
    ['\u015f', '\u015e', '\u0000', '\u0000'], // AC11
    ['\u0078', '\u0058', '\u00b4', '\u0000'], // BKSL
    ['\u003c', '\u003e', '\u007c', '\u0000'], // LSGT
    ['\u006a', '\u004a', '\u00ab', '\u0000'], // AB01
    ['\u00f6', '\u00d6', '\u00bb', '\u0000'], // AB02
    ['\u0076', '\u0056', '\u00a2', '\u0000'], // AB03
    ['\u0063', '\u0043', '\u0000', '\u0000'], // AB04
    ['\u00e7', '\u00c7', '\u0000', '\u0000'], // AB05
    ['\u007a', '\u005a', '\u0000', '\u0000'], // AB06
    ['\u0073', '\u0053', '\u00b5', '\u0000'], // AB07
    ['\u0062', '\u0042', '\u00d7', '\u0000'], // AB08
    ['\u002e', '\u003a', '\u00f7', '\u0000'], // AB09
    ['\u002c', '\u003b', '\u0000', '\u0000'], // AB10
    ['\u0020', '\u0020', '\u0000', '\u0000'] // SPCE
  ];

  var data = i18n.input.chrome.inputview.content.util.createData(
      keyCharacters, viewIdPrefix_, true, true);
  data['id'] = 'tr-f';
  google.ime.chrome.inputview.onConfigLoaded(data);
}) ();
