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
    ['\u0030', '\u00a7', '\u00ac', '\u0000'], // TLDE
    ['\u0031', '\u0027', '\u007e', '\u0303'], // AE01
    ['\u0032', '\u0022', '\u030c', '\u02c7'], // AE02
    ['\u0033', '\u002b', '\u005e', '\u0302'], // AE03
    ['\u0034', '\u0021', '\u0306', '\u02d8'], // AE04
    ['\u0035', '\u0025', '\u030a', '\u00b0'], // AE05
    ['\u0036', '\u002f', '\u0328', '\u02db'], // AE06
    ['\u0037', '\u003d', '\u0060', '\u0300'], // AE07
    ['\u0038', '\u0028', '\u0307', '\u02d9'], // AE08
    ['\u0039', '\u0029', '\u0301', '\u00b4'], // AE09
    ['\u00f6', '\u00d6', '\u030b', '\u02dd'], // AE10
    ['\u00fc', '\u00dc', '\u0308', '\u00a8'], // AE11
    ['\u00f3', '\u00d3', '\u0327', '\u00b8'], // AE12

    ['\u0071', '\u0051', '\u005c', '\u0000'], // AD01
    ['\u0077', '\u0057', '\u007c', '\u0000'], // AD02
    ['\u0065', '\u0045', '\u201e', '\u00bb'], // AD03
    ['\u0072', '\u0052', '\u201d', '\u00ab'], // AD04
    ['\u0074', '\u0054', '\u0000', '\u0000'], // AD05
    ['\u0079', '\u0059', '\u0000', '\u0000'], // AD06
    ['\u0075', '\u0055', '\u20ac', '\u0000'], // AD07
    ['\u0069', '\u0049', '\u00cd', '\u00ed'], // AD08
    ['\u006f', '\u004f', '\u0000', '\u0000'], // AD09
    ['\u0070', '\u0050', '\u0000', '\u0000'], // AD10
    ['\u0151', '\u0150', '\u00f7', '\u0000'], // AD11
    ['\u00fa', '\u00da', '\u00d7', '\u0000'], // AD12

    ['\u0061', '\u0041', '\u00e4', '\u00c4'], // AC01
    ['\u0073', '\u0053', '\u0111', '\u0000'], // AC02
    ['\u0064', '\u0044', '\u0110', '\u0000'], // AC03
    ['\u0066', '\u0046', '\u005b', '\u0000'], // AC04
    ['\u0067', '\u0047', '\u005d', '\u0000'], // AC05
    ['\u0068', '\u0048', '\u00ed', '\u00cd'], // AC06
    ['\u006a', '\u004a', '\u0027', '\u031b'], // AC07
    ['\u006b', '\u004b', '\u0142', '\u0000'], // AC08
    ['\u006c', '\u004c', '\u0141', '\u0000'], // AC09
    ['\u00e9', '\u00c9', '\u0141', '\u0000'], // AC10
    ['\u00e1', '\u00c1', '\u00df', '\u0000'], // AC11
    ['\u0171', '\u0170', '\u00a4', '\u0000'], // BKSL
    ['\u00ed', '\u00cd', '\u003c', '\u003e'], // LSGT

    ['\u007a', '\u005a', '\u003e', '\u0000'], // AB01
    ['\u0078', '\u0058', '\u0023', '\u0000'], // AB02
    ['\u0063', '\u0043', '\u0026', '\u0000'], // AB03
    ['\u0076', '\u0056', '\u0040', '\u0000'], // AB04
    ['\u0062', '\u0042', '\u007b', '\u0000'], // AB05
    ['\u006e', '\u004e', '\u007d', '\u0000'], // AB06
    ['\u006d', '\u004d', '\u003c', '\u0000'], // AB07
    ['\u002c', '\u003f', '\u003b', '\u0000'], // AB08
    ['\u002e', '\u003a', '\u003e', '\u0000'], // AB09
    ['\u002d', '\u005f', '\u002a', '\u0000'], // AB10
    ['\u0020', '\u0020', '\u0000', '\u0000']  // SPCE
  ];

  var data = i18n.input.chrome.inputview.content.util.createData(
      keyCharacters, viewIdPrefix_, true, true);
  data['id'] = 'hu-qwerty';
  google.ime.chrome.inputview.onConfigLoaded(data);
}) ();
