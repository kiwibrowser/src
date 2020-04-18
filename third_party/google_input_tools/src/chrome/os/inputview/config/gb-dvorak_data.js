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
    ['\u0060', '\u00ac', '\u007c', '\u007c'], // TLDE
    ['\u0031', '\u0021'], // AE01
    ['\u0032', '\u0022', '\u00b2', '\u0000'], // AE02
    ['\u0033', '\u00a3', '\u00b3', '\u0000'], // AE03
    ['\u0034', '\u0024', '\u20ac', '\u0000'], // AE04
    ['\u0035', '\u0025'], // AE05
    ['\u0036', '\u005e', '\u0302', '\u0302'], // AE06
    ['\u0037', '\u0026'], // AE07
    ['\u0038', '\u002a'], // AE08
    ['\u0039', '\u0028', '\u0300', '\u0000'], // AE09
    ['\u0030', '\u0029'], // AE10
    ['\u005b', '\u007b'], // AE11
    ['\u005d', '\u007d', '\u0303', '\u0000'], // AE12
    ['\u0027', '\u0040', '\u0301', '\u0308'], // AD01
    ['\u002c', '\u003c', '\u00e4', '\u030c'], // AD02
    ['\u002e', '\u003e', '\u00ea', '\u00b7'], // AD03
    ['\u0070', '\u0050', '\u00eb', '\u0327'], // AD04
    ['\u0079', '\u0059', '\u00fc', '\u0000'], // AD05
    ['\u0066', '\u0046'], // AD06
    ['\u0067', '\u0047'], // AD07
    ['\u0063', '\u0043', '\u00e7', '\u0307'], // AD08
    ['\u0072', '\u0052'], // AD09
    ['\u006c', '\u004c'], // AD10
    ['\u002f', '\u003f'], // AD11
    ['\u003d', '\u002b'], // AD12
    ['\u0061', '\u0041', '\u00e0', '\u0000'], // AC01
    ['\u006f', '\u004f', '\u00f4', '\u0000'], // AC02
    ['\u0065', '\u0045', '\u00e9', '\u0000'], // AC03
    ['\u0075', '\u0055', '\u00fb', '\u0000'], // AC04
    ['\u0069', '\u0049', '\u00ee', '\u0000'], // AC05
    ['\u0064', '\u0044'], // AC06
    ['\u0068', '\u0048'], // AC07
    ['\u0074', '\u0054'], // AC08
    ['\u006e', '\u004e'], // AC09
    ['\u0073', '\u0053', '\u00df', '\u0000'], // AC10
    ['\u002d', '\u005f'], // AC11
    ['\u0023', '\u007e'], // BKSL
    ['\u005c', '\u007c', '\u007c', '\u00a6'], // LSGT
    ['\u003b', '\u003a', '\u00e2', '\u030b'], // AB01
    ['\u0071', '\u0051', '\u00f6', '\u0328'], // AB02
    ['\u006a', '\u004a', '\u00e8', '\u030b'], // AB03
    ['\u006b', '\u004b', '\u00f9', '\u0000'], // AB04
    ['\u0078', '\u0058', '\u00ef', '\u0000'], // AB05
    ['\u0062', '\u0042'], // AB06
    ['\u006d', '\u004d'], // AB07
    ['\u0077', '\u0057'], // AB08
    ['\u0076', '\u0056'], // AB09
    ['\u007a', '\u005a'], // AB10
    ['\u0020', '\u0020'] // SPCE
  ];

  var data = i18n.input.chrome.inputview.content.util.createData(
      keyCharacters, viewIdPrefix_, true, true);
  data['id'] = 'gb-dvorak';
  google.ime.chrome.inputview.onConfigLoaded(data);
}) ();
