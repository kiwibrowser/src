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
goog.require('goog.array');
goog.require('i18n.input.chrome.ElementType');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.Direction');
goog.require('i18n.input.chrome.inputview.SpecNodeName');
goog.require('i18n.input.chrome.inputview.content.util');

(function() {
  var Css = i18n.input.chrome.inputview.Css;
  var Direction = i18n.input.chrome.inputview.Direction;
  var ElementType = i18n.input.chrome.ElementType;
  var SpecNodeName = i18n.input.chrome.inputview.SpecNodeName;
  var util = i18n.input.chrome.inputview.content.util;


  /**
 * The physical key codes for Japanese keyboard.
 *
 * @type {!Array.<string>}
 */
  var keyIds = [
    'Digit1',
    'Digit2',
    'Digit3',
    'Digit4',
    'Digit5',
    'Digit6',
    'Digit7',
    'Digit8',
    'Digit9',
    'Digit0',
    'Minus',
    'Equal',
    'IntlYen',

    'KeyQ',
    'KeyW',
    'KeyE',
    'KeyR',
    'KeyT',
    'KeyY',
    'KeyU',
    'KeyI',
    'KeyO',
    'KeyP',
    'BracketLeft',
    'BracketRight',

    'KeyA',
    'KeyS',
    'KeyD',
    'KeyF',
    'KeyG',
    'KeyH',
    'KeyJ',
    'KeyK',
    'KeyL',
    'Semicolon',
    'Quote',
    'Backslash',

    'KeyZ',
    'KeyX',
    'KeyC',
    'KeyV',
    'KeyB',
    'KeyN',
    'KeyM',
    'Comma',
    'Period',
    'Slash',
    'IntlRo'
  ];


  /**
 * Creates the Japanese keyset data.
 *
 * @param {!Array.<!Array.<string>>} keyCharacters The key characters.
 * @param {string} viewIdPrefix The prefix of the view.
 * @param {!Array.<!Array.<number>>} keyCodes The key codes.
 * @return {!Object} The key data.
 */
  var createData = function(keyCharacters, viewIdPrefix, keyCodes) {
    var keyList = [];
    keyList.push(util.createIMESwitchKey('Toggle', '英漢', Css.JP_IME_SWITCH));
    // The keys shows the shift character in Default state. In material design,
    // Only the first 11 keys will show shift character.
    var keysShowShift = 11;
    goog.array.forEach(keyCharacters, function(c, i) {
      var spec = {};
      spec[SpecNodeName.ID] = keyIds[i];
      spec[SpecNodeName.TYPE] = ElementType.CHARACTER_KEY;
      spec[SpecNodeName.CHARACTERS] = c;
      spec[SpecNodeName.KEY_CODE] = keyCodes[i];
      if (i < keysShowShift) {
        spec[SpecNodeName.ENABLE_SHIFT_RENDERING] = true;
      }
      var key = util.createKey(spec);
      keyList.push(key);
    });

    goog.array.insertAt(keyList, util.createBackspaceKey(), 14);
    goog.array.insertAt(keyList, util.createTabKey(), 15);
    goog.array.insertAt(keyList, util.createCapslockKey(), 28);
    goog.array.insertAt(keyList, util.createEnterKey(), 41);
    goog.array.insertAt(keyList, util.createShiftKey(true), 42);
    keyList.push(util.createShiftKey(false));
    keyList.push(util.createGlobeKey());
    keyList.push(util.createMenuKey());
    keyList.push(util.createCtrlKey());
    keyList.push(util.createAltKey());
    keyList.push(util.createIMESwitchKey(
        'NonConvert', '英数', Css.JP_IME_SWITCH));
    keyList.push(util.createSpaceKey());
    keyList.push(util.createIMESwitchKey('Convert', 'かな', Css.JP_IME_SWITCH));
    keyList.push(util.createArrowKey(Direction.LEFT));
    keyList.push(util.createArrowKey(Direction.RIGHT));
    keyList.push(util.createHideKeyboardKey());

    var mapping = {};
    goog.array.forEach(keyList, function(key, i) {
      mapping[key['spec'][SpecNodeName.ID]] = viewIdPrefix + i;
    });

    var result = {};
    result[SpecNodeName.KEY_LIST] = keyList;
    result[SpecNodeName.MAPPING] = mapping;
    result[SpecNodeName.LAYOUT] = 'jpkbd';
    result[SpecNodeName.HAS_ALTGR_KEY] = false;
    result[SpecNodeName.SHOW_MENU_KEY] = true;
    return result;
  };

  var viewIdPrefix_ = 'jpkbd-k-';
  var jpKeyCharacters = [
    ['1', '!'], // AE01
    ['2', '"'], // AE02
    ['3', '#'], // AE03
    ['4', '$'], // AE04
    ['5', '%'], // AE05
    ['6', '&'], // AE06
    ['7', '\''],// AE07
    ['8', '('], // AE08
    ['9', ')'], // AE09
    ['0', '0'], // AE10
    ['-', '='], // AE11
    ['^', '~'], // AE12
    ['\\', '|'], // IntlYen

    ['q', 'Q'], // AD01
    ['w', 'W'], // AD02
    ['e', 'E'], // AD03
    ['r', 'R'], // AD04
    ['t', 'T'], // AD05
    ['y', 'Y'], // AD06
    ['u', 'U'], // AD07
    ['i', 'I'], // AD08
    ['o', 'O'], // AD09
    ['p', 'P'], // AD10
    ['@', '`'], // AD11
    ['[', '{'], // AD12

    ['a', 'A'], // AC01
    ['s', 'S'], // AC02
    ['d', 'D'], // AC03
    ['f', 'F'], // AC04
    ['g', 'G'], // AC05
    ['h', 'H'], // AC06
    ['j', 'J'], // AC07
    ['k', 'K'], // AC08
    ['l', 'L'], // AC09
    [';', '+'], // AC10
    [':', '*'], // AC11
    [']', '}'], // BKSL

    ['z', 'Z'], // AB01
    ['x', 'X'], // AB02
    ['c', 'C'], // AB03
    ['v', 'V'], // AB04
    ['b', 'B'], // AB05
    ['n', 'N'], // AB06
    ['m', 'M'], // AB07
    [',', '<'], // AB08
    ['.', '>'], // AB09
    ['/', '?'], // AB10
    ['\\', '_'] // IntlRo
  ]; // 46


  var keyCodes = [
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
    0xFF, // IntlYen

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
    0xC1  // IntlRo
  ]; // 46
  var jpData = createData(jpKeyCharacters, viewIdPrefix_, keyCodes);
  jpData['id'] = 'jp';
  google.ime.chrome.inputview.onConfigLoaded(jpData);


  // Creates kana keyset.
  var kanaKeyCharacters = [
    ['\u306C'],  // 'ぬ', AE01
    ['\u3075'],  // 'ふ', AE02
    ['\u3042', '\u3041'],  // 'あ', 'ぁ', AE03
    ['\u3046', '\u3045'],  // 'う', 'ぅ', AE04
    ['\u3048', '\u3047'],  // 'え', 'ぇ', AE05
    ['\u304A', '\u3049'],  // 'お', 'ぉ', AE06
    ['\u3084', '\u3083'],  // 'や', 'ゃ', AE07
    ['\u3086', '\u3085'],  // 'ゆ', 'ゅ', AE08
    ['\u3088', '\u3087'],  // 'よ', 'ょ', AE09
    ['\u308F', '\u3092'],  // 'わ', 'を', AE10
    ['\u307B'],  // 'ほ', AE11
    ['\u3078'],  // 'へ', AE12
    ['\u30FC'],  // 'ー', IntlYen

    ['\u305F'],  // 'た', AD01
    ['\u3066'],  // 'て', AD02
    ['\u3044', '\u3043'],  // 'い', 'ぃ', AD03
    ['\u3059'],  // 'す', AD04
    ['\u304B'],  // 'か', AD05
    ['\u3093'],  // 'ん', AD06
    ['\u306A'],  // 'な', AD07
    ['\u306B'],  // 'に', AD08
    ['\u3089'],  // 'ら', AD09
    ['\u305B'],  // 'せ', AD10
    ['\u309B'],  // '゛', AD11
    ['\u309C', '\u300C'],  // '゜', '「', AD12

    ['\u3061'],  // 'ち', AC01
    ['\u3068'],  // 'と', AC02
    ['\u3057'],  // 'し', AC03
    ['\u306F'],  // 'は', AC04
    ['\u304D'],  // 'き', AC05
    ['\u304F'],  // 'く', AC06
    ['\u307E'],  // 'ま', AC07
    ['\u306E'],  // 'の', AC08
    ['\u308A'],  // 'り', AC09
    ['\u308C'],  // 'れ', AC10
    ['\u3051'],  // 'け', AC11
    ['\u3080', '\u300D'],  // 'む', '」', BKSL

    ['\u3064', '\u3063'],  // 'つ', 'っ', AB01
    ['\u3055'],  // 'さ', AB02
    ['\u305D'],  // 'そ', AB03
    ['\u3072'],  // 'ひ', AB04
    ['\u3053'],  // 'こ', AB05
    ['\u307F'],  // 'み', AB06
    ['\u3082'],  // 'も', AB07
    ['\u306D', '\u3001'],  // 'ね', '、', AB08
    ['\u308B', '\u3002'],  // 'る', '。', AB09
    ['\u3081', '\u30FB'],  // 'め', '・', AB10
    ['\u308D']  // 'ろ', IntlRo
  ]; // 46

  var kanaData = createData(kanaKeyCharacters, viewIdPrefix_, keyCodes);
  kanaData['id'] = 'jp-kana';
  google.ime.chrome.inputview.onConfigLoaded(kanaData);
})(); // goog.scope;
