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
    ['\u0060', '\u007e', '\u0303', '\u007e'], // TLDE
    ['\u0031', '\u0021', '\u00a1', '\u00b9'], // AE01
    ['\u0032', '\u0040', '\u00ba', '\u00b2'], // AE02
    ['\u0033', '\u0023', '\u00aa', '\u00b3'], // AE03
    ['\u0034', '\u0024', '\u00a2', '\u00a3'], // AE04
    ['\u0035', '\u0025', '\u20ac', '\u00a5'], // AE05
    ['\u0036', '\u005e', '\u0127', '\u0126'], // AE06
    ['\u0037', '\u0026', '\u00f0', '\u00d0'], // AE07
    ['\u0038', '\u002a', '\u00fe', '\u00de'], // AE08
    ['\u0039', '\u0028', '\u2018', '\u201c'], // AE09
    ['\u0030', '\u0029', '\u2019', '\u201d'], // AE10
    ['\u002d', '\u005f', '\u2013', '\u2014'], // AE11
    ['\u003d', '\u002b', '\u00d7', '\u00f7'], // AE12
    ['\u0071', '\u0051', '\u00e4', '\u00c4'], // AD01
    ['\u0077', '\u0057', '\u00e5', '\u00c5'], // AD02
    ['\u0066', '\u0046', '\u00e3', '\u00c3'], // AD03
    ['\u0070', '\u0050', '\u00f8', '\u00d8'], // AD04
    ['\u0067', '\u0047', '\u0328', '\u007e'], // AD05
    ['\u006a', '\u004a', '\u0111', '\u0110'], // AD06
    ['\u006c', '\u004c', '\u0142', '\u0141'], // AD07
    ['\u0075', '\u0055', '\u00fa', '\u00da'], // AD08
    ['\u0079', '\u0059', '\u00fc', '\u00dc'], // AD09
    ['\u003b', '\u003a', '\u00f6', '\u00d6'], // AD10
    ['\u005b', '\u007b', '\u00ab', '\u2039'], // AD11
    ['\u005d', '\u007d', '\u00bb', '\u203a'], // AD12
    ['\u005c', '\u007c', '\u007e', '\u007e'], // BKSL
    ['\u0061', '\u0041', '\u00e1', '\u00c1'], // AC01
    ['\u0072', '\u0052', '\u0300', '\u007e'], // AC02
    ['\u0073', '\u0053', '\u00df', '\u007e'], // AC03
    ['\u0074', '\u0054', '\u0301', '\u030b'], // AC04
    ['\u0064', '\u0044', '\u0308', '\u007e'], // AC05
    ['\u0068', '\u0048', '\u030c', '\u007e'], // AC06
    ['\u006e', '\u004e', '\u00f1', '\u00d1'], // AC07
    ['\u0065', '\u0045', '\u00e9', '\u00c9'], // AC08
    ['\u0069', '\u0049', '\u00ed', '\u00cd'], // AC09
    ['\u006f', '\u004f', '\u00f3', '\u00d3'], // AC10
    ['\u0027', '\u0022', '\u00f5', '\u00d5'], // AC11
    ['\u007a', '\u005a', '\u00e6', '\u00c6'], // AB01
    ['\u0078', '\u0058', '\u0302', '\u007e'], // AB02
    ['\u0063', '\u0043', '\u00e7', '\u00c7'], // AB03
    ['\u0076', '\u0056', '\u0153', '\u0152'], // AB04
    ['\u0062', '\u0042', '\u0306', '\u007e'], // AB05
    ['\u006b', '\u004b', '\u030a', '\u007e'], // AB06
    ['\u006d', '\u004d', '\u0304', '\u007e'], // AB07
    ['\u002c', '\u003c', '\u0327', '\u007e'], // AB08
    ['\u002e', '\u003e', '\u0307', '\u007e'], // AB09
    ['\u002f', '\u003f', '\u00bf', '\u007e'], // AB10
    ['\u0020', '\u0020', '\u0020', '\u00a0'] // SPCE
  ];

  var data = i18n.input.chrome.inputview.content.util.createData(
      keyCharacters, viewIdPrefix_, false, true);
  data['id'] = 'us-colemak';
  google.ime.chrome.inputview.onConfigLoaded(data);
}) ();
