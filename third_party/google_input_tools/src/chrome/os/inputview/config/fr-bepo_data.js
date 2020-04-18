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
    ['\u0024', '\u0023', '\u2013', '\u00b6'], // TLDE
    ['\u0022', '\u0031', '\u2014', '\u201e'], // AE01
    ['\u00ab', '\u0032', '\u003c', '\u201c'], // AE02
    ['\u00bb', '\u0033', '\u003e', '\u201d'], // AE03
    ['\u0028', '\u0034', '\u005b', '\u2264'], // AE04
    ['\u0029', '\u0035', '\u005d', '\u2265'], // AE05
    ['\u0040', '\u0036', '\u005e', '\u0000'], // AE06
    ['\u002b', '\u0037', '\u00b1', '\u00ac'], // AE07
    ['\u002d', '\u0038', '\u2212', '\u00bc'], // AE08
    ['\u002f', '\u0039', '\u00f7', '\u00bd'], // AE09
    ['\u002a', '\u0030', '\u00d7', '\u00be'], // AE10
    ['\u003d', '\u00b0', '\u2260', '\u2032'], // AE11
    ['\u0025', '\u0060', '\u2030', '\u2033'], // AE12
    ['\u0062', '\u0042', '\u007c', '\u00a6'], // AD01
    ['\u00e9', '\u00c9', '\u0301', '\u030b'], // AD02
    ['\u0070', '\u0050', '\u0026', '\u00a7'], // AD03
    ['\u006f', '\u004f', '\u0153', '\u0152'], // AD04
    ['\u00e8', '\u00c8', '\u0300', '\u0060'], // AD05
    ['\u005e', '\u0021', '\u00a1', '\u0000'], // AD06
    ['\u0076', '\u0056', '\u030c', '\u0000'], // AD07
    ['\u0064', '\u0044', '\u00f0', '\u00d0'], // AD08
    ['\u006c', '\u004c', '\u002f', '\u0000'], // AD09
    ['\u006a', '\u004a', '\u0133', '\u0132'], // AD10
    ['\u007a', '\u005a', '\u0259', '\u018f'], // AD11
    ['\u0077', '\u0057', '\u0306', '\u0000'], // AD12
    ['\u0061', '\u0041', '\u00e6', '\u00c6'], // AC01
    ['\u0075', '\u0055', '\u00f9', '\u00d9'], // AC02
    ['\u0069', '\u0049', '\u0308', '\u0307'], // AC03
    ['\u0065', '\u0045', '\u20ac', '\u00a4'], // AC04
    ['\u002c', '\u003b', '\u2019', '\u031b'], // AC05
    ['\u0063', '\u0043', '\u00a9', '\u017f'], // AC06
    ['\u0074', '\u0054', '\u00fe', '\u00de'], // AC07
    ['\u0073', '\u0053', '\u00df', '\u1e9e'], // AC08
    ['\u0072', '\u0052', '\u00ae', '\u2122'], // AC09
    ['\u006e', '\u004e', '\u0303', '\u0000'], // AC10
    ['\u006d', '\u004d', '\u0304', '\u00ba'], // AC11
    ['\u00e7', '\u00c7', '\u0327', '\u002c'], // BKSL
    ['\u00ea', '\u00ca', '\u002f', '\u0000'], // LSGT
    ['\u00e0', '\u00c0', '\u005c', '\u0000'], // AB01
    ['\u0079', '\u0059', '\u007b', '\u2018'], // AB02
    ['\u0078', '\u0058', '\u007d', '\u2019'], // AB03
    ['\u002e', '\u003a', '\u2026', '\u00b7'], // AB04
    ['\u006b', '\u004b', '\u007e', '\u0000'], // AB05
    ['\u0027', '\u003f', '\u00bf', '\u0309'], // AB06
    ['\u0071', '\u0051', '\u030a', '\u0323'], // AB07
    ['\u0067', '\u0047', '\u00b5', '\u0000'], // AB08
    ['\u0068', '\u0048', '\u2020', '\u2021'], // AB09
    ['\u0066', '\u0046', '\u0328', '\u00aa'], // AB10
    ['\u0020', '\u00a0', '\u005f', '\u202f']  // SPCE
  ];

  var data = i18n.input.chrome.inputview.content.util.createData(
      keyCharacters, viewIdPrefix_, true, true);
  data['id'] = 'fr-bepo';
  google.ime.chrome.inputview.onConfigLoaded(data);
}) ();

