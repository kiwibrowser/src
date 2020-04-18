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
    ['\u055d', '\u055c'], // TLDE
    ['\u0567', '\u0537'], // AE01
    ['\u0569', '\u0539'], // AE02
    ['\u0583', '\u0553'], // AE03
    ['\u0571', '\u0541'], // AE04
    ['\u057b', '\u054b'], // AE05
    ['\u0582', '\u0552'], // AE06
    ['\u0587', '\u0587'], // AE07
    ['\u057c', '\u054c'], // AE08
    ['\u0579', '\u0549'], // AE09
    ['\u0573', '\u0543'], // AE10
    ['\u002d', '\u2015'], // AE11
    ['\u056a', '\u053a'], // AE12
    ['\u0584', '\u0554'], // AD01
    ['\u0578', '\u0548'], // AD02
    ['\u0565', '\u0535'], // AD03
    ['\u0580', '\u0550'], // AD04
    ['\u057f', '\u054f'], // AD05
    ['\u0568', '\u0538'], // AD06
    ['\u0582', '\u0552'], // AD07
    ['\u056b', '\u053b'], // AD08
    ['\u0585', '\u0555'], // AD09
    ['\u057a', '\u054a'], // AD10
    ['\u056d', '\u053d'], // AD11
    ['\u056e', '\u053e'], // AD12
    ['\u0577', '\u0547'], // BKSL
    ['\u0561', '\u0531'], // AC01
    ['\u057d', '\u054d'], // AC02
    ['\u0564', '\u0534'], // AC03
    ['\u0586', '\u0556'], // AC04
    ['\u0563', '\u0533'], // AC05
    ['\u0570', '\u0540'], // AC06
    ['\u0575', '\u0545'], // AC07
    ['\u056f', '\u053f'], // AC08
    ['\u056c', '\u053c'], // AC09
    ['\u003b', '\u0589'], // AC10
    ['\u055b', '\u0022'], // AC11
    ['\u0566', '\u0536'], // AB01
    ['\u0572', '\u0542'], // AB02
    ['\u0581', '\u0551'], // AB03
    ['\u057e', '\u054e'], // AB04
    ['\u0562', '\u0532'], // AB05
    ['\u0576', '\u0546'], // AB06
    ['\u0574', '\u0544'], // AB07
    ['\u002c', '\u00ab'], // AB08
    ['\u2024', '\u00bb'], // AB09
    ['\u002f', '\u055e'], // AB10
    ['\u0020', '\u0020'] // SPCE
  ];

  var data = i18n.input.chrome.inputview.content.util.createData(
      keyCharacters, viewIdPrefix_, false, false);
  data['id'] = 'am-phonetic';
  google.ime.chrome.inputview.onConfigLoaded(data);
}) ();
