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
goog.provide('i18n.input.chrome.inputview.content.compact.symbol');

goog.require('i18n.input.chrome.inputview.content.Constants');

goog.scope(function() {
var NON_LETTER_KEYS =
    i18n.input.chrome.inputview.content.Constants.NON_LETTER_KEYS;


/**
 * Gets North American Symbol keyset characters.
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.symbol.keyNASymbolCharacters =
    function() {
  return [
    /* 0 */ { 'text': '1',
      'moreKeys': {
        'characters': ['\u00B9', '\u00BD', '\u2153', '\u00BC', '\u215B']}},
    /* 1 */ { 'text': '2',
      'moreKeys': {
        'characters': ['\u00B2', '\u2154']}},
    /* 2 */ { 'text': '3',
      'moreKeys': {
        'characters': ['\u00B3', '\u00BE', '\u215C']}},
    /* 3 */ { 'text': '4',
      'moreKeys': {
        'characters': ['\u2074']}},
    /* 4 */ { 'text': '5',
      'moreKeys': {
        'characters': ['\u215D']}},
    /* 5 */ { 'text': '6' },
    /* 6 */ { 'text': '7',
      'moreKeys': {
        'characters': ['\u215E']}},
    /* 7 */ { 'text': '8' },
    /* 8 */ { 'text': '9' },
    /* 9 */ { 'text': '0',
      'moreKeys': {
        'characters': ['\u207F', '\u2205']}},
    /* 10 */ NON_LETTER_KEYS.BACKSPACE,
    /* 11 */ { 'text': '@', 'marginLeftPercent': 0.33 },
    /* 12 */ { 'text': '#' },
    /* 13 */ { 'text': '$',
      'moreKeys': {
        'characters': ['\u00A2', '\u00A3', '\u20AC', '\u00A5', '\u20B1']}},
    /* 14 */ { 'text': '%',
      'moreKeys': {
        'characters': ['\u2030']}},
    /* 15 */ { 'text': '&' },
    // Keep in sync with rowkeys_symbols2.xml in android input tool.
    /* 16 */ { 'text': '-',
      'moreKeys': {
        'characters': ['_', '\u2013', '\u2014', '\u00B7']}},
    /* 17 */ { 'text': '+',
      'moreKeys': {
        'characters': ['\u00B1']}},
    /* 18 */ { 'text': '(',
      'moreKeys': {
        'characters': ['<', '{', '[']}},
    /* 19 */ { 'text': ')',
      'moreKeys': {
        'characters': ['>', '}', ']']}},
    /* 20 */ NON_LETTER_KEYS.ENTER,
    /* 21 */ NON_LETTER_KEYS.SWITCHER,
    /* 22 */ { 'text': '\\' },
    /* 23 */ { 'text': '=' },
    /* 24 */ { 'text': '*',
      'moreKeys': {
        'characters': ['\u2020', '\u2021', '\u2605']}},
    /* 25 */ { 'text': '"',
      'moreKeys': {
        'characters': ['\u201E', '\u201C', '\u201D', '\u00AB', '\u00BB']}},
    /* 26 */ { 'text': '\'',
      'moreKeys': {
        'characters': ['\u201A', '\u2018', '\u2019', '\u2039', '\u203A']}},
    /* 27 */ { 'text': ':' },
    /* 28 */ { 'text': ';' },
    /* 29 */ { 'text': '!',
      'moreKeys': {
        'characters': ['\u00A1']}},
    /* 30 */ { 'text': '?',
      'moreKeys': {
        'characters': ['\u00BF']}},
    /* 31 */ NON_LETTER_KEYS.SWITCHER,
    /* 32 */ NON_LETTER_KEYS.SWITCHER,
    /* 33 */ { 'text': '_', 'isGrey': true },
    /* 34 */ NON_LETTER_KEYS.MENU,
    /* 35 */ { 'text': '/', 'isGrey': true },
    /* 36 */ NON_LETTER_KEYS.SPACE,
    /* 37 */ { 'text': ',', 'isGrey': true },
    // Keep in sync with row_symbols4.xml in android input tool.
    /* 38 */ { 'text': '.', 'isGrey': true,
      'moreKeys': {
        'characters': ['\u2026']}},
    /* 39 */ NON_LETTER_KEYS.HIDE
  ];
};


/**
 * Gets United Kingdom Symbol keyset characters.
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.symbol.keyUKSymbolCharacters =
    function() {
  // Copy North America symbol characters.
  var data = i18n.input.chrome.inputview.content.compact.symbol.
      keyNASymbolCharacters();
  // UK uses pound sign instead of dollar sign.
  data[13] = { 'text': '\u00A3',
               'moreKeys': {
                 'characters': ['\u00A2', '$', '\u20AC', '\u00A5', '\u20B1']}};
  return data;
};


/**
 * Gets European Symbol keyset characters.
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.symbol.keyEUSymbolCharacters =
    function() {
  // Copy UK symbol characters.
  var data = i18n.input.chrome.inputview.content.compact.symbol.
      keyUKSymbolCharacters();
  // European uses euro sign instead of pound sign.
  data[13] = { 'text': '\u20AC',
               'moreKeys': {
                 'characters': ['\u00A2', '$', '\u00A3', '\u00A5', '\u20B1']}};
  return data;
};


/**
 * Gets Pinyin Symbol keyset characters.
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.symbol.keyPinyinSymbolCharacters =
    function() {
  var data = i18n.input.chrome.inputview.content.compact.symbol.
      keyNASymbolCharacters();
  data[13]['text'] = '\u00A5';
  data[13]['moreKeys'] = {
        'characters': ['\u0024', '\u00A2', '\u00A3', '\u20AC', '\u20B1']};
  data[15]['text'] = '&';
  data[18]['text'] = '\uff08';
  data[18]['moreKeys'] = {
        'characters': ['\uff5b', '\u300a', '\uff3b', '\u3010']};
  data[19]['text'] = '\uff09';
  data[19]['moreKeys'] = {
        'characters': ['\uff5d', '\u300b', '\uff3d', '\u3001']};
  data[22]['text'] = '\u3001';
  data[25]['text'] = '\u201C';
  data[25]['moreKeys'] = {
        'characters': ['\u0022', '\u00AB']};
  data[26]['text'] = '\u201D';
  data[26]['moreKeys'] = {
        'characters': ['\u0022', '\u00BB']};
  data[27]['text'] = '\uff1a';
  data[28]['text'] = '\uff1b';
  data[29]['text'] = '\u2018';
  data[29]['moreKeys'] = {
        'characters': ['\u0027', '\u2039']};
  data[30]['text'] = '\u2019';
  data[30]['moreKeys'] = {
        'characters': ['\u0027', '\u203a']};
  data[33]['text'] = ' \u2014';
  data[33]['moreKeys'] = undefined;
  data[35]['text'] = '\u2026';
  data[35]['moreKeys'] = undefined;
  data[37]['text'] = '\uff0c';
  data[38]['text'] = '\u3002';
  return data;
};
});  // goog.scope
