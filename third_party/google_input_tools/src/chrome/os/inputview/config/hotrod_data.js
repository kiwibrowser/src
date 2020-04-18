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
goog.require('i18n.input.chrome.inputview.SpecNodeName');
goog.require('i18n.input.chrome.inputview.content.Constants');
goog.require('i18n.input.chrome.inputview.content.compact.util');
goog.require('i18n.input.chrome.inputview.content.compact.util.CompactKeysetSpec');

(function() {
  var NON_LETTER_KEYS =
      i18n.input.chrome.inputview.content.Constants.NON_LETTER_KEYS;
  var keysetSpecNode =
      i18n.input.chrome.inputview.content.compact.util.CompactKeysetSpec;
  var SpecNodeName = i18n.input.chrome.inputview.SpecNodeName;

  var hotrodKeysetSpec = {};
  hotrodKeysetSpec[keysetSpecNode.ID] = 'hotrod';
  hotrodKeysetSpec[keysetSpecNode.LAYOUT] = 'hotrod';
  hotrodKeysetSpec[keysetSpecNode.DATA] = [
    /* 0 */ { 'text': 'q' },
    /* 1 */ { 'text': 'w' },
    /* 2 */ { 'text': 'e' },
    /* 3 */ { 'text': 'r' },
    /* 4 */ { 'text': 't' },
    /* 5 */ { 'text': 'y' },
    /* 6 */ { 'text': 'u' },
    /* 7 */ { 'text': 'i' },
    /* 8 */ { 'text': 'o' },
    /* 9 */ { 'text': 'p' },
    /* 10 */ NON_LETTER_KEYS.BACKSPACE,
    /* 11 */ { 'text': '7'},
    /* 12 */ { 'text': '8'},
    /* 13 */ { 'text': '9'},
    /* 14 */ { 'text': 'a', 'marginLeftPercent': 0.33 },
    /* 15 */ { 'text': 's' },
    /* 16 */ { 'text': 'd' },
    /* 17 */ { 'text': 'f' },
    /* 18 */ { 'text': 'g' },
    /* 19 */ { 'text': 'h' },
    /* 20 */ { 'text': 'j' },
    /* 21 */ { 'text': 'k' },
    /* 22 */ { 'text': 'l' },
    /* 23 */ NON_LETTER_KEYS.ENTER,
    /* 24 */ { 'text': '4'},
    /* 25 */ { 'text': '5'},
    /* 26 */ { 'text': '6'},
    /* 27 */ { 'text': 'z', 'marginLeftPercent': 0.67 },
    /* 28 */ { 'text': 'x' },
    /* 29 */ { 'text': 'c' },
    /* 30 */ { 'text': 'v' },
    /* 31 */ { 'text': 'b' },
    /* 32 */ { 'text': 'n' },
    /* 33 */ { 'text': 'm', 'marginRightPercent': 0.72 },
    /* 34 */ { 'text': '1'},
    /* 35 */ { 'text': '2'},
    /* 36 */ { 'text': '3'},
    /* 37 */ NON_LETTER_KEYS.HOTROD_SWITCHER,
    /* 38 */ { 'text': '@' },
    /* 39 */ { 'text': '-' },
    /* 40 */ { 'text': '/' },
    /* 41 */ { 'text': '_' },
    /* 42 */ { 'text': '.' },
    /* 43 */ { 'text': '.com' },
    /* 44 */ { 'text': '.org' },
    /* 45 */ { 'text': '.net' },
    /* 46 */ { 'text': '.edu', 'marginRightPercent': 0.5 },
    /* 47 */ { 'text': '0' }
  ];

  var data = i18n.input.chrome.inputview.content.compact.util.createCompactData(
      hotrodKeysetSpec, 'hotrod-k-', 'hotrod-k-key-');
  data[SpecNodeName.ID] = hotrodKeysetSpec[keysetSpecNode.ID];
  data[SpecNodeName.NO_SHIFT] = true;
  google.ime.chrome.inputview.onConfigLoaded(data);
}) ();
