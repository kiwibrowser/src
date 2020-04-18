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
goog.provide('i18n.input.chrome.inputview.content.compact.letter');

goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.MoreKeysShiftOperation');
goog.require('i18n.input.chrome.inputview.content.Constants');

goog.scope(function() {
var NON_LETTER_KEYS =
    i18n.input.chrome.inputview.content.Constants.NON_LETTER_KEYS;
var HINT_TEXT_PLACE_HOLDER =
    i18n.input.chrome.inputview.content.Constants.HINT_TEXT_PLACE_HOLDER;
var MoreKeysShiftOperation = i18n.input.chrome.inputview.MoreKeysShiftOperation;
var Css = i18n.input.chrome.inputview.Css;


/**
 * Common qwerty Letter keyset characters.
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.letter.keyQwertyCharacters =
    function() {
  return [
    /* 0 */ { 'text': 'q', 'hintText': '1' },
    /* 1 */ { 'text': 'w', 'hintText': '2' },
    /* 2 */ { 'text': 'e', 'hintText': '3',
      'moreKeys': {
        'characters': ['\u00E9', '\u00E8', '\u00EA', '\u00EB', '\u0113']}},
    /* 3 */ { 'text': 'r', 'hintText': '4' },
    /* 4 */ { 'text': 't', 'hintText': '5' },
    /* 5 */ { 'text': 'y', 'hintText': '6' },
    /* 6 */ { 'text': 'u', 'hintText': '7',
      'moreKeys': {
        'characters': ['\u00FA', '\u00FB', '\u00FC', '\u00F9', '\u016B']}},
    /* 7 */ { 'text': 'i', 'hintText': '8',
      'moreKeys': {
        'characters': ['\u00ED', '\u00EE', '\u00EF', '\u012B', '\u00EC']}},
    /* 8 */ { 'text': 'o', 'hintText': '9',
      'moreKeys': {
        'characters': ['\u00F3', '\u00F4', '\u00F6', '\u00F2', '\u0153',
          '\u00F8', '\u014D', '\u00F5']}},
    /* 9 */ { 'text': 'p', 'hintText': '0' },
    /* 10 */ NON_LETTER_KEYS.BACKSPACE,
    /* 11 */ { 'text': 'a', 'marginLeftPercent': 0.33,
      'moreKeys': {
        'characters': ['\u00E0', '\u00E1', '\u00E2', '\u00E4', '\u00E6',
          '\u00E3', '\u00E5', '\u0101']}},
    /* 12 */ { 'text': 's',
      'moreKeys': {'characters': ['\u00DF']}},
    /* 13 */ { 'text': 'd' },
    /* 14 */ { 'text': 'f' },
    /* 15 */ { 'text': 'g' },
    /* 16 */ { 'text': 'h' },
    /* 17 */ { 'text': 'j' },
    /* 18 */ { 'text': 'k' },
    /* 19 */ { 'text': 'l' },
    /* 20 */ NON_LETTER_KEYS.ENTER,
    /* 21 */ NON_LETTER_KEYS.LEFT_SHIFT,
    /* 22 */ { 'text': 'z' },
    /* 23 */ { 'text': 'x' },
    /* 24 */ { 'text': 'c',
      'moreKeys': {'characters': ['\u00E7']}},
    /* 25 */ { 'text': 'v' },
    /* 26 */ { 'text': 'b' },
    /* 27 */ { 'text': 'n',
      'moreKeys': {'characters': ['\u00F1']}},
    /* 28 */ { 'text': 'm' },
    /* 29 */ { 'text': '!',
      'moreKeys': {'characters': ['\u00A1']}},
    /* 30 */ { 'text': '?',
      'moreKeys': {'characters': ['\u00BF']}},
    /* 31 */ NON_LETTER_KEYS.RIGHT_SHIFT,
    /* 32 */ NON_LETTER_KEYS.SWITCHER,
    /* 33 */ NON_LETTER_KEYS.GLOBE,
    /* 34 */ NON_LETTER_KEYS.MENU,
    /* 35 */ { 'text': '/', 'isGrey': true, 'onContext':
          { 'email' : { 'text' : '@' }}},
    /* 36 */ NON_LETTER_KEYS.SPACE,
    /* 37 */ { 'text': ',', 'isGrey': true, 'onContext':
          { 'email' : {'text' : '.com', 'textCssClass' : Css.FONT_SMALL,
              'moreKeys': {'characters': ['.net', '.org']}},
          'url' : {'text' : '.com', 'textCssClass' : Css.FONT_SMALL,
              'moreKeys': {'characters': ['.net', '.org']}}}},
    /* 38 */ { 'text': '.', 'isGrey': true,
      'moreKeys': {
        'characters': [',', '\'', '#', ')', '(', '/', ';', '@', ':',
          '-', '"', '+', '%', '&'],
        'fixedColumnNumber': 7}},
    /* 39 */ NON_LETTER_KEYS.HIDE
  ];
};


/**
 * Nederland Letter keyset characters.
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.letter.keyNederlandCharacters =
    function() {
  var data =
      i18n.input.chrome.inputview.content.compact.letter.keyQwertyCharacters();
  data[2]['moreKeys'] = {'characters': ['\u00E9', '\u00EB', '\u00EA', '\u00E8',
    '\u0119', '\u0117', '\u0113']};  // e
  data[5]['moreKeys'] = {'characters': ['\u0133']};  // y
  data[6]['moreKeys'] = {'characters':
        ['\u00FA', '\u00FC', '\u00FB', '\u00F9', '\u016B']};  // u
  data[7]['moreKeys'] = {'characters': ['\u00ED', '\u00EF', '\u00EC', '\u00EE',
    '\u012F', '\u012B', '\u0133']};  // i
  data[8]['moreKeys'] = {'characters': ['\u00F3', '\u00F6', '\u00F4', '\u00F2',
    '\u00F5', '\u0153', '\u00F8', '\u014D']};  // o
  data[11]['moreKeys'] = {'characters': ['\u00E1', '\u00E4', '\u00E2', '\u00E0',
    '\u00E6', '\u00E3', '\u00E5', '\u0101']}; // a
  data[12]['moreKeys'] = undefined;  // s
  data[24]['moreKeys'] = undefined;  // c
  data[27]['moreKeys'] = {'characters': ['\u00F1', '\u0144']};  // n
  return data;
};


/**
 * Icelandic Letter keyset characters.
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.letter.keyIcelandicCharacters =
    function() {
  var data =
      i18n.input.chrome.inputview.content.compact.letter.keyQwertyCharacters();
  data[2]['moreKeys'] = {'characters': ['\u00E9', '\u00EB', '\u00E8', '\u00EA',
    '\u0119', '\u0117', '\u0113']};  // e
  data[4]['moreKeys'] = {'characters': ['\u00FE']};  // t
  data[5]['moreKeys'] = {'characters': ['\u00FD', '\u00FF']}; // y
  data[6]['moreKeys'] = {'characters':
        ['\u00FA', '\u00FC', '\u00FB', '\u00F9', '\u016B']};  // u
  data[7]['moreKeys'] = {'characters':
        ['\u00ED', '\u00EF', '\u00EE', '\u00EC', '\u012F', '\u012B']};  // i
  data[8]['moreKeys'] = {'characters': ['\u00F3', '\u00F6', '\u00F4', '\u00F2',
    '\u00F5', '\u0153', '\u00F8', '\u014D']};  // o
  data[11]['moreKeys'] = {'characters': ['\u00E1', '\u00E4', '\u00E6', '\u00E5',
    '\u00E0', '\u00E2', '\u00E3', '\u0101']}; // a
  data[12]['moreKeys'] = undefined;  // s
  data[13]['moreKeys'] = {'characters': ['\u00F0']};  // d
  data[24]['moreKeys'] = undefined;  // c
  data[27]['moreKeys'] = undefined;  // n
  return data;
};


/**
 * Qwertz Germany Letter keyset characters.
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.letter.keyQwertzCharacters =
    function() {
  var data =
      i18n.input.chrome.inputview.content.compact.letter.keyQwertyCharacters();
  data[2]['moreKeys'] = {'characters':
        ['\u00E9', '\u00E8', '\u00EA', '\u00EB', '\u0117']};  // e
  data[6]['moreKeys'] = {
    'characters': ['\u00FC', HINT_TEXT_PLACE_HOLDER, '\u00FB', '\u00F9',
      '\u00FA', '\u016B']};  // u
  data[7]['moreKeys'] = undefined;  // i
  data[8]['moreKeys'] = {
    'characters': ['\u00F6', HINT_TEXT_PLACE_HOLDER, '\u00F4', '\u00F2',
      '\u00F3', '\u00F5', '\u0153', '\u00F8', '\u014D']};  // o
  data[11]['moreKeys'] = {
    'characters': ['\u00E4', HINT_TEXT_PLACE_HOLDER, '\u00E2', '\u00E0',
      '\u00E1', '\u00E6', '\u00E3', '\u00E5', '\u0101']}; // a
  data[12]['moreKeys'] = {'characters': ['\u00DF', '\u015B', '\u0161']};  // s
  data[24]['moreKeys'] = undefined;  // c
  data[27]['moreKeys'] = {'characters': ['\u00F1', '\u0144']};

  data[5].text = 'z';
  data[22].text = 'y';
  return data;
};


/**
 * Common azerty Letter keyset characters.
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.letter.keyAzertyCharacters =
    function() {
  return [
    /* 0 */ { 'text': 'a', 'hintText': '1',
      'moreKeys': {
        'characters': ['\u00E0', '\u00E2', HINT_TEXT_PLACE_HOLDER, '\u00E6',
          '\u00E1', '\u00E4', '\u00E3', '\u00E5', '\u0101', '\u00AA']}},
    /* 1 */ { 'text': 'z', 'hintText': '2' },
    /* 2 */ { 'text': 'e', 'hintText': '3',
      'moreKeys': {
        'characters': ['\u00E9', '\u00E8', '\u00EA', '\u00EB',
          HINT_TEXT_PLACE_HOLDER, '\u0119', '\u0117', '\u0113']}},
    /* 3 */ { 'text': 'r', 'hintText': '4' },
    /* 4 */ { 'text': 't', 'hintText': '5' },
    /* 5 */ { 'text': 'y', 'hintText': '6',
      'moreKeys': {'characters': ['\u00FF']}},
    /* 6 */ { 'text': 'u', 'hintText': '7',
      'moreKeys': {
        'characters': ['\u00F9', '\u00FB', HINT_TEXT_PLACE_HOLDER, '\u00FC',
          '\u00FA', '\u016B']}},
    /* 7 */ { 'text': 'i', 'hintText': '8',
      'moreKeys': {
        'characters': ['\u00EE', HINT_TEXT_PLACE_HOLDER, '\u00EF', '\u00EC',
          '\u00ED', '\u012F', '\u012B']}},
    /* 8 */ { 'text': 'o', 'hintText': '9',
      'moreKeys': {
        'characters': ['\u00F4', '\u0153', HINT_TEXT_PLACE_HOLDER, '\u00F6',
          '\u00F2', '\u00F3', '\u00F5', '\u00F8', '\u014D', '\u00BA']}},
    /* 9 */ { 'text': 'p', 'hintText': '0' },
    /* 10 */ NON_LETTER_KEYS.BACKSPACE,
    /* 11 */ { 'text': 'q' },
    /* 12 */ { 'text': 's' },
    /* 13 */ { 'text': 'd' },
    /* 14 */ { 'text': 'f' },
    /* 15 */ { 'text': 'g' },
    /* 16 */ { 'text': 'h' },
    /* 17 */ { 'text': 'j' },
    /* 18 */ { 'text': 'k' },
    /* 19 */ { 'text': 'l' },
    /* 20 */ { 'text': 'm' },
    /* 21 */ NON_LETTER_KEYS.ENTER,
    /* 22 */ NON_LETTER_KEYS.LEFT_SHIFT,
    /* 23 */ { 'text': 'w' },
    /* 24 */ { 'text': 'x' },
    /* 25 */ { 'text': 'c',
      'moreKeys': {
        'characters': ['\u00E7', '\u0107', '\u010D']}},
    /* 26 */ { 'text': 'v' },
    /* 27 */ { 'text': 'b' },
    /* 28 */ { 'text': 'n' },
    /* 29 */ { 'text': '\'',
      'moreKeys': {
        'characters': ['\u201A', '\u2018', '\u2019', '\u2039', '\u203A']}},
    /* 30 */ { 'text': '!',
      'moreKeys': {
        'characters': ['\u00A1']}},
    /* 31 */ { 'text': '?',
      'moreKeys': {
        'characters': ['\u00BF']}},
    /* 32 */ NON_LETTER_KEYS.RIGHT_SHIFT,
    /* 33 */ NON_LETTER_KEYS.SWITCHER,
    /* 34 */ NON_LETTER_KEYS.GLOBE,
    /* 35 */ NON_LETTER_KEYS.MENU,
    /* 36 */ { 'text': '/', 'isGrey': true },
    /* 37 */ NON_LETTER_KEYS.SPACE,
    /* 38 */ { 'text': ',', 'isGrey': true },
    /* 39 */ { 'text': '.', 'isGrey': true,
      'moreKeys': {
        'characters': [',', '\'', '#', ')', '(', '/', ';', '@', ':',
          '-', '"', '+', '%', '&'],
        'fixedColumnNumber': 7}},
    /* 40 */ NON_LETTER_KEYS.HIDE
  ];
};


/**
 * Basic nordic Letter keyset characters(based on Finish).
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.letter.keyNordicCharacters =
    function() {
  return [
    /* 0 */ { 'text': 'q', 'hintText': '1' },
    /* 1 */ { 'text': 'w', 'hintText': '2' },
    /* 2 */ { 'text': 'e', 'hintText': '3' },
    /* 3 */ { 'text': 'r', 'hintText': '4' },
    /* 4 */ { 'text': 't', 'hintText': '5' },
    /* 5 */ { 'text': 'y', 'hintText': '6' },
    /* 6 */ { 'text': 'u', 'hintText': '7',
      'moreKeys': {
        'characters': ['\u00FC']}},
    /* 7 */ { 'text': 'i', 'hintText': '8' },
    /* 8 */ { 'text': 'o', 'hintText': '9',
      'moreKeys': {
        'characters': ['\u00F8', '\u00F4', '\u00F2', '\u00F3', '\u00F5',
          '\u0153', '\u014D']}},
    /* 9 */ { 'text': 'p', 'hintText': '0' },
    /* 10 */ { 'text': '\u00e5' },
    /* 11 */ NON_LETTER_KEYS.BACKSPACE,
    /* 12 */ { 'text': 'a',
      'moreKeys': {
        'characters': ['\u00E6', '\u00E0', '\u00E1', '\u00E2', '\u00E3',
          '\u0101']}},
    /* 13 */ { 'text': 's',
      'moreKeys': {
        'characters': ['\u0161', '\u00DF', '\u015B']}},
    /* 14 */ { 'text': 'd' },
    /* 15 */ { 'text': 'f' },
    /* 16 */ { 'text': 'g' },
    /* 17 */ { 'text': 'h' },
    /* 18 */ { 'text': 'j' },
    /* 19 */ { 'text': 'k' },
    /* 20 */ { 'text': 'l' },
    /* 21 */ { 'text': '\u00f6',
      'moreKeys': {
        'characters': ['\u00F8']}},
    /* 22 */ { 'text': '\u00e4',
      'moreKeys': {
        'characters': ['\u00E6']}},
    /* 23 */ NON_LETTER_KEYS.ENTER,
    /* 24 */ NON_LETTER_KEYS.LEFT_SHIFT,
    /* 25 */ { 'text': 'z', 'marginLeftPercent': 0.33,
      'moreKeys': {
        'characters': ['\u017E', '\u017A', '\u017C']}},
    /* 26 */ { 'text': 'x' },
    /* 27 */ { 'text': 'c' },
    /* 28 */ { 'text': 'v' },
    /* 29 */ { 'text': 'b' },
    /* 30 */ { 'text': 'n' },
    /* 31 */ { 'text': 'm' },
    /* 32 */ { 'text': '!',
      'moreKeys': {
        'characters': ['\u00A1']}},
    /* 33 */ { 'text': '?', 'marginRightPercent': 0.33,
      'moreKeys': {
        'characters': ['\u00BF']}},
    /* 34 */ NON_LETTER_KEYS.RIGHT_SHIFT,
    /* 35 */ NON_LETTER_KEYS.SWITCHER,
    /* 36 */ NON_LETTER_KEYS.GLOBE,
    /* 37 */ NON_LETTER_KEYS.MENU,
    /* 38 */ { 'text': '/', 'isGrey': true },
    /* 39 */ NON_LETTER_KEYS.SPACE,
    /* 40 */ { 'text': ',', 'isGrey': true },
    /* 41 */ { 'text': '.', 'isGrey': true,
      'moreKeys': {
        'characters': [',', '\'', '#', ')', '(', '/', ';', '@', ':',
          '-', '"', '+', '%', '&'],
        'fixedColumnNumber': 7}},
    /* 42 */ NON_LETTER_KEYS.HIDE
  ];
};


/**
 * Sweden nordic Letter keyset characters.
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.letter.keySwedenCharacters =
    function() {
  var data =
      i18n.input.chrome.inputview.content.compact.letter.keyNordicCharacters();
  data[2]['moreKeys'] = {
    'characters': ['\u00E9', '\u00E8', '\u00EA', '\u00EB', '\u0119']};  // e
  data[3]['moreKeys'] = {
    'characters': ['\u0159']};  // r
  data[4]['moreKeys'] = {
    'characters': ['\u0165', '\u00FE']};  // t
  data[5]['moreKeys'] = {
    'characters': ['\u00FD', '\u00FF']};  // y
  data[6]['moreKeys'] = {
    'characters': ['\u00FC', '\u00FA', '\u00F9', '\u00FB', '\u016B']};  // u
  data[7]['moreKeys'] = {
    'characters': ['\u00ED', '\u00EC', '\u00EE', '\u00EF']};  // i
  data[8]['moreKeys'] = {
    'characters': ['\u00F3', '\u00F2', '\u00F4', '\u00F5', '\u014D']};  // o
  data[12]['moreKeys'] = {
    'characters': ['\u00E1', '\u00E0', '\u00E2', '\u0105', '\u00E3']};  // a
  data[13]['moreKeys'] = {
    'characters': ['\u015B', '\u0161', '\u015F', '\u00DF']};  // s
  data[14]['moreKeys'] = {
    'characters': ['\u00F0', '\u010F']};  // d
  data[20]['moreKeys'] = {
    'characters': ['\u0142']};  // l
  data[21]['moreKeys'] = {
    'characters': ['\u00F8', '\u0153']};
  data[25]['moreKeys'] = {
    'characters': ['\u017A', '\u017E', '\u017C']};  //z
  data[27]['moreKeys'] = {
    'characters': ['\u00E7', '\u0107', '\u010D']};  //c
  data[30]['moreKeys'] = {
    'characters': ['\u0144', '\u00F1', '\u0148']};  //n

  return data;
};


/**
 * Norway nordic Letter keyset characters.
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.letter.keyNorwayCharacters =
    function() {
  var data =
      i18n.input.chrome.inputview.content.compact.letter.keyNordicCharacters();
  data[2]['moreKeys'] = {
    'characters': ['\u00E9', '\u00E8', '\u00EA', '\u00EB', '\u0119', '\u0117',
      '\u0113']};  // e
  data[6]['moreKeys'] = {
    'characters': ['\u00FC', '\u00FB', '\u00F9', '\u00FA', '\u016B']};  // u
  data[8]['moreKeys'] = {
    'characters': ['\u00F4', '\u00F2', '\u00F3', '\u00F6', '\u00F5', '\u0153',
      '\u014D']};  // o
  data[12]['moreKeys'] = {
    'characters': ['\u00E0', '\u00E4', '\u00E1', '\u00E2', '\u00E3',
      '\u0101']};  // a
  data[13]['moreKeys'] = undefined;  //s
  data[21]['moreKeys'] = {
    'characters': ['\u00F6']};
  data[22]['moreKeys'] = {
    'characters': ['\u00E4']};
  data[25]['moreKeys'] = undefined;  //z

  data[21]['text'] = '\u00f8';
  data[22]['text'] = '\u00e6';

  return data;
};


/**
 * Denmark nordic Letter keyset characters.
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.letter.keyDenmarkCharacters =
    function() {
  var data =
      i18n.input.chrome.inputview.content.compact.letter.keyNordicCharacters();
  data[2]['moreKeys'] = {
    'characters': ['\u00E9', '\u00EB']};  // e
  data[5]['moreKeys'] = {
    'characters': ['\u00FD', '\u00FF']};  // y
  data[6]['moreKeys'] = {
    'characters': ['\u00FA', '\u00FC', '\u00FB', '\u00F9', '\u016B']};  // u
  data[7]['moreKeys'] = {
    'characters': ['\u00ED', '\u00EF']};  // i
  data[8]['moreKeys'] = {
    'characters': ['\u00F3', '\u00F4', '\u00F2', '\u00F5', '\u0153',
      '\u014D']};  // o
  data[12]['moreKeys'] = {
    'characters': ['\u00E1', '\u00E4', '\u00E0', '\u00E2', '\u00E3',
      '\u0101']};  // a
  data[13]['moreKeys'] = {
    'characters': ['\u00DF', '\u015B', '\u0161']};  // s
  data[14]['moreKeys'] = {
    'characters': ['\u00F0']};  // d
  data[20]['moreKeys'] = {
    'characters': ['\u0142']};  // l
  data[21]['moreKeys'] = {
    'characters': ['\u00E4']};
  data[22]['moreKeys'] = {
    'characters': ['\u00F6']};
  data[25]['moreKeys'] = undefined;  //z
  data[30]['moreKeys'] = {
    'characters': ['\u00F1', '\u0144']};  //n

  data[21]['text'] = '\u00e6';
  data[22]['text'] = '\u00f8';

  return data;
};


/**
 * Pinyin keyset characters.
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.letter.keyPinyinCharacters =
    function() {
  var data = [
    /* 0 */ { 'text': 'q', 'hintText': '1',
      'moreKeys': {
        'characters': ['\u0051', '\u0071']}},
    /* 1 */ { 'text': 'w', 'hintText': '2',
      'moreKeys': {
        'characters': ['\u0057', '\u0077']}},
    /* 2 */ { 'text': 'e', 'hintText': '3',
      'moreKeys': {
        'characters': ['\u0045', '\u0065']}},
    /* 3 */ { 'text': 'r', 'hintText': '4',
      'moreKeys': {
        'characters': ['\u0052', '\u0072']}},
    /* 4 */ { 'text': 't', 'hintText': '5',
      'moreKeys': {
        'characters': ['\u0054', '\u0074']}},
    /* 5 */ { 'text': 'y', 'hintText': '6',
      'moreKeys': {
        'characters': ['\u0059', '\u0079']}},
    /* 6 */ { 'text': 'u', 'hintText': '7',
      'moreKeys': {
        'characters': ['\u0055', '\u0075']}},
    /* 7 */ { 'text': 'i', 'hintText': '8',
      'moreKeys': {
        'characters': ['\u0049', '\u0069']}},
    /* 8 */ { 'text': 'o', 'hintText': '9',
      'moreKeys': {
        'characters': ['\u004F', '\u006F']}},
    /* 9 */ { 'text': 'p', 'hintText': '0',
      'moreKeys': {
        'characters': ['\u0050', '\u0070']}},
    /* 10 */ NON_LETTER_KEYS.BACKSPACE,
    /* 11 */ { 'text': 'a', 'hintText': '@', 'marginLeftPercent': 0.33,
      'moreKeys': {
        'characters': ['\u0041', '\u0061']}},
    /* 12 */ { 'text': 's', 'hintText': '*',
      'moreKeys': {
        'characters': ['\u0053', '\u0073']}},
    /* 13 */ { 'text': 'd', 'hintText': '+',
      'moreKeys': {
        'characters': ['\u0044', '\u0064']}},
    /* 14 */ { 'text': 'f', 'hintText': '-',
      'moreKeys': {
        'characters': ['\u0046', '\u0066']}},
    /* 15 */ { 'text': 'g', 'hintText': '=',
      'moreKeys': {
        'characters': ['\u0047', '\u0067']}},
    /* 16 */ { 'text': 'h', 'hintText': '/',
      'moreKeys': {
        'characters': ['\u0048', '\u0068']}},
    /* 17 */ { 'text': 'j', 'hintText': '#',
      'moreKeys': {
        'characters': ['\u004a', '\u006a']}},
    /* 18 */ { 'text': 'k', 'hintText': '\uff08',
      'moreKeys': {
        'characters': ['\u004b', '\u006b']}},
    /* 19 */ { 'text': 'l', 'hintText': '\uff09',
      'moreKeys': {
        'characters': ['\u004c', '\u006c']}},
    /* 20 */ NON_LETTER_KEYS.ENTER,
    /* 21 */ NON_LETTER_KEYS.LEFT_SHIFT,
    /* 22 */ { 'text': 'z', 'hintText': '\u3001',
      'moreKeys': {
        'characters': ['\u005a', '\u007a']}},
    /* 23 */ { 'text': 'x', 'hintText': '\uff1a',
      'moreKeys': {
        'characters': ['\u0058', '\u0078']}},
    /* 24 */ { 'text': 'c', 'hintText': '\"',
      'moreKeys': {
        'characters': ['\u0043', '\u0063']}},
    /* 25 */ { 'text': 'v', 'hintText': '\uff1f',
      'moreKeys': {
        'characters': ['\u0056', '\u0076']}},
    /* 26 */ { 'text': 'b', 'hintText': '\uff01',
      'moreKeys': {
        'characters': ['\u0042', '\u0062']}},
    /* 27 */ { 'text': 'n', 'hintText': '\uff5e',
      'moreKeys': {
        'characters': ['\u004e', '\u006e']}},
    /* 28 */ { 'text': 'm', 'hintText': '.',
      'moreKeys': {
        'characters': ['\u004d', '\u006d']}},
    /* 29 */ { 'text': '\uff01', 'hintText': '%',
      'moreKeys': {
        'characters': ['\u00A1']}},
    /* 30 */ { 'text': '\uff1f', 'hintText': '&',
      'moreKeys': {
        'characters': ['\u00BF']}},
    /* 31 */ NON_LETTER_KEYS.RIGHT_SHIFT,
    /* 32 */ NON_LETTER_KEYS.SWITCHER,
    /* 33 */ NON_LETTER_KEYS.GLOBE,
    /* 34 */ NON_LETTER_KEYS.MENU,
    /* 35 */ { 'text': '\uff0c', 'isGrey': true },
    /* 36 */ NON_LETTER_KEYS.SPACE,
    /* 37 */ { 'text': '\u3002', 'isGrey': true },
    /* 38 */ NON_LETTER_KEYS.SWITCHER,
    /* 39 */ NON_LETTER_KEYS.HIDE
  ];
  for (var i = 0; i <= 9; i++) {
    data[i]['moreKeysShiftOperation'] = MoreKeysShiftOperation.TO_LOWER_CASE;
  }
  for (var i = 11; i <= 19; i++) {
    data[i]['moreKeysShiftOperation'] = MoreKeysShiftOperation.TO_LOWER_CASE;
  }
  for (var i = 22; i <= 28; i++) {
    data[i]['moreKeysShiftOperation'] = MoreKeysShiftOperation.TO_LOWER_CASE;
  }
  return data;
};


/**
 * English mode of pinyin keyset characters.
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.letter.keyEnCharacters =
    function() {
  var data =
      i18n.input.chrome.inputview.content.compact.letter.keyPinyinCharacters();
  for (var i = 0; i <= 9; i++) {
    data[i]['moreKeys']['characters'].pop();
  }
  for (var i = 11; i <= 19; i++) {
    data[i]['moreKeys']['characters'].pop();
  }
  for (var i = 22; i <= 28; i++) {
    data[i]['moreKeys']['characters'].pop();
  }
  data[12]['hintText'] = '*';
  data[14]['hintText'] = '\u002d';
  data[16]['hintText'] = '/';
  data[18]['hintText'] = '\u0028';
  data[19]['hintText'] = '\u0029';
  data[22]['hintText'] = '\u0027';
  data[23]['hintText'] = '\u003a';
  data[25]['hintText'] = '\u003f';
  data[26]['hintText'] = '\u0021';
  data[27]['hintText'] = '\u007e';
  data[28]['hintText'] = '\u2026';
  data[29]['text'] = '\u0021';
  data[30]['text'] = '\u003f';
  data[35]['text'] = '\u002c';
  data[37]['text'] = '.';
  return data;
};


/**
 * Zhuyin keyset characters.
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.letter.keyZhuyinCharacters =
    function() {
  var data = [
    /* 0 */ { 'text': '\u3105', 'hintText': '1',
      'moreKeys': {
        'characters': ['\uff01']}},
    /* 1 */ { 'text': '\u3109', 'hintText': '2',
      'moreKeys': {
        'characters': ['@']}},
    /* 2 */ { 'text': '\u02c7', 'hintText': '3',
      'moreKeys': {
        'characters': ['#']}},
    /* 3 */ { 'text': '\u02cb', 'hintText': '4',
      'moreKeys': {
        'characters': ['$']}},
    /* 4 */ { 'text': '\u3113', 'hintText': '5',
      'moreKeys': {
        'characters': ['%']}},
    /* 5 */ { 'text': '\u02ca', 'hintText': '6',
      'moreKeys': {
        'characters': ['^']}},
    /* 6 */ { 'text': '\u02d9', 'hintText': '7',
      'moreKeys': {
        'characters': ['&']}},
    /* 7 */ { 'text': '\u311a', 'hintText': '8',
      'moreKeys': {
        'characters': ['*']}},
    /* 8 */ { 'text': '\u311e', 'hintText': '9',
      'moreKeys': {
        'characters': ['\uff08']}},
    /* 9 */ { 'text': '\u3122', 'hintText': '0',
      'moreKeys': {
        'characters': ['\uff09']}},
    /* 10 */ { 'text': '\u3106', 'hintText': 'q',
      'moreKeys': {
        'characters': ['Q'], 'textCode': ['Q']}},
    /* 11 */ { 'text': '\u310a', 'hintText': 'w',
      'moreKeys': {
        'characters': ['W'], 'codeKeys': ['Q']}},
    /* 12 */ { 'text': '\u310d', 'hintText': 'e',
      'moreKeys': {
        'characters': ['E'], 'textCode': ['Q']}},
    /* 13 */ { 'text': '\u3110', 'hintText': 'r',
      'moreKeys': {
        'characters': ['R']}},
    /* 14 */ { 'text': '\u3114', 'hintText': 't',
      'moreKeys': {
        'characters': ['T']}},
    /* 15 */ { 'text': '\u3117', 'hintText': 'y',
      'moreKeys': {
        'characters': ['Y']}},
    /* 16 */ { 'text': '\u3127', 'hintText': 'u',
      'moreKeys': {
        'characters': ['U']}},
    /* 17 */ { 'text': '\u311b', 'hintText': 'i',
      'moreKeys': {
        'characters': ['I']}},
    /* 18 */ { 'text': '\u311f', 'hintText': 'o',
      'moreKeys': {
        'characters': ['O']}},
    /* 19 */ { 'text': '\u3123', 'hintText': 'p',
      'moreKeys': {
        'characters': ['P']}},

    /* 20 */ { 'text': '\u3107', 'hintText': 'a',
      'moreKeys': {
        'characters': ['A']}},
    /* 21 */ { 'text': '\u310B', 'hintText': 's',
      'moreKeys': {
        'characters': ['S']}},
    /* 22 */ { 'text': '\u310e', 'hintText': 'd',
      'moreKeys': {
        'characters': ['D']}},
    /* 23 */ { 'text': '\u3111', 'hintText': 'f',
      'moreKeys': {
        'characters': ['F']}},
    /* 24 */ { 'text': '\u3115', 'hintText': 'g',
      'moreKeys': {
        'characters': ['G']}},
    /* 25 */ { 'text': '\u3118', 'hintText': 'h',
      'moreKeys': {
        'characters': ['H']}},
    /* 26 */ { 'text': '\u3128', 'hintText': 'j',
      'moreKeys': {
        'characters': ['J']}},
    /* 27 */ { 'text': '\u311c', 'hintText': 'k',
      'moreKeys': {
        'characters': ['K']}},
    /* 28 */ { 'text': '\u3120', 'hintText': 'l',
      'moreKeys': {
        'characters': ['L']}},
    /* 29 */ { 'text': '\u3124', 'hintText': '\uff1a'},

    /* 30 */ { 'text': '\u3108', 'hintText': 'z',
      'moreKeys': {
        'characters': ['Z']}},
    /* 31 */ { 'text': '\u310c', 'hintText': 'x',
      'moreKeys': {
        'characters': ['X']}},
    /* 32 */ { 'text': '\u310f', 'hintText': 'c',
      'moreKeys': {
        'characters': ['C']}},
    /* 33 */ { 'text': '\u3112', 'hintText': 'v',
      'moreKeys': {
        'characters': ['V']}},
    /* 34 */ { 'text': '\u3116', 'hintText': 'b',
      'moreKeys': ['B'] },
    /* 35 */ { 'text': '\u3119', 'hintText': 'n',
      'moreKeys': {
        'characters': ['N']}},
    /* 36 */ { 'text': '\u3129', 'hintText': 'm',
      'moreKeys': {
        'characters': ['M']}},
    /* 37 */ { 'text': '\u311d', 'hintText': '\u2026'},
    /* 38 */ { 'text': '\u3121', 'hintText': '\uff01'},
    /* 39 */ { 'text': '\u3125', 'hintText': '\uff1f'},

    /* 40 */ NON_LETTER_KEYS.BACKSPACE,
    /* 41 */ NON_LETTER_KEYS.ENTER,
    /* 42 */ NON_LETTER_KEYS.RIGHT_SHIFT,

    /* 43 */ NON_LETTER_KEYS.SWITCHER,
    /* 44 */ NON_LETTER_KEYS.GLOBE,
    /* 45 */ NON_LETTER_KEYS.MENU,
    /* 46 */ { 'text': '\uff0c', 'isGrey': true },
    /* 47 */ NON_LETTER_KEYS.SPACE,
    /* 48 */ { 'text': '\u3126', 'isGrey': false },
    /* 49 */ { 'text': '\u3002', 'isGrey': true },
    /* 50 */ NON_LETTER_KEYS.SWITCHER,
    /* 51 */ NON_LETTER_KEYS.HIDE
  ];
  for (var i = 0; i <= 39; i++) {
    data[i]['moreKeysShiftOperation'] = MoreKeysShiftOperation.TO_LOWER_CASE;
  }
  for (var i = 0; i <= 39; i++) {
    data[i]['onShift'] = (data[i]['hintText']).toUpperCase();
  }
  return data;
};
});  // goog.scope
