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
goog.provide('i18n.input.chrome.inputview.content.compact.more');

goog.require('i18n.input.chrome.inputview.content.Constants');

goog.scope(function() {
var NON_LETTER_KEYS =
    i18n.input.chrome.inputview.content.Constants.NON_LETTER_KEYS;


/**
 * North American More keyset characters.
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.more.keyNAMoreCharacters =
    function() {
  return [
    /* 0 */ { 'text': '~' },
    /* 1 */ { 'text': '`' },
    /* 2 */ { 'text': '|' },
    // Keep in sync with rowkeys_symbols_shift1.xml in android input tool.
    /* 3 */ { 'text': '\u2022',
      'moreKeys': {
        'characters': ['\u266A', '\u2665', '\u2660', '\u2666', '\u2663']}},
    /* 4 */ { 'text': '\u23B7' },
    // Keep in sync with rowkeys_symbols_shift1.xml in android input tool.
    /* 5 */ { 'text': '\u03C0',
      'moreKeys': {
        'characters': ['\u03A0']}},
    /* 6 */ { 'text': '\u00F7' },
    /* 7 */ { 'text': '\u00D7' },
    /* 8 */ { 'text': '\u00B6',
      'moreKeys': {
        'characters': ['\u00A7']}},
    /* 9 */ { 'text': '\u0394' },
    /* 10 */ NON_LETTER_KEYS.BACKSPACE,
    /* 11 */ { 'text': '\u00A3', 'marginLeftPercent': 0.33 },
    /* 12 */ { 'text': '\u00A2' },
    /* 13 */ { 'text': '\u20AC' },
    /* 14 */ { 'text': '\u00A5' },
    // Keep in sync with rowkeys_symbols_shift2.xml in android input tool.
    /* 15 */ { 'text': '^',
      'moreKeys': {
        'characters': ['\u2191', '\u2193', '\u2190', '\u2192']}},
    // Keep in sync with rowkeys_symbols_shift2.xml in android input tool.
    /* 16 */ { 'text': '\u00B0',
      'moreKeys': {
        'characters': ['\u2032', '\u2033']}},
    // Keep in sync with rowkeys_symbols_shift2.xml in android input tool.
    /* 17 */ { 'text': '=',
      'moreKeys': {
        'characters': ['\u2260', '\u2248', '\u221E']}},
    /* 18 */ { 'text': '{' },
    /* 19 */ { 'text': '}' },
    /* 20 */ NON_LETTER_KEYS.ENTER,
    /* 21 */ NON_LETTER_KEYS.SWITCHER,
    /* 22 */ { 'text': '\\' },
    /* 23 */ { 'text': '\u00A9' },
    /* 24 */ { 'text': '\u00AE' },
    /* 25 */ { 'text': '\u2122' },
    /* 26 */ { 'text': '\u2105' },
    /* 27 */ { 'text': '[' },
    /* 28 */ { 'text': ']' },
    /* 29 */ { 'text': '\u00A1' },
    /* 30 */ { 'text': '\u00BF' },
    /* 31 */ NON_LETTER_KEYS.SWITCHER,
    /* 32 */ NON_LETTER_KEYS.SWITCHER,
    // Keep in sync with row_symbols_shift4.xml in android input tool.
    /* 33 */ { 'text': '<', 'isGrey': true,
      'moreKeys': {
        'characters': ['\u2039', '\u2264', '\u00AB']}},
    /* 34 */ NON_LETTER_KEYS.MENU,
    // Keep in sync with row_symbols_shift4.xml in android input tool.
    /* 35 */ { 'text': '>', 'isGrey': true,
      'moreKeys': {
        'characters': ['\u203A', '\u2265', '\u00BB']}},
    /* 36 */ NON_LETTER_KEYS.SPACE,
    /* 37 */ { 'text': ',', 'isGrey': true },
    // Keep in sync with row_symbols_shift4.xml in android input tool.
    /* 38 */ { 'text': '.', 'isGrey': true,
      'moreKeys': {
        'characters': ['\u2026']}},
    /* 39 */ NON_LETTER_KEYS.HIDE
  ];
};


/**
 * Gets United Kingdom More keyset characters.
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.more.keyUKMoreCharacters =
    function() {
  // Copy North America more characters.
  var data = i18n.input.chrome.inputview.content.compact.more.
      keyNAMoreCharacters();
  data[11]['text'] = '\u20AC';  // pound -> euro
  data[12]['text'] = '\u00A5';  // cent -> yen
  data[13]['text'] = '$';  // euro -> dollar
  data[13]['moreKeys'] = {
        'characters': ['\u00A2']};
  data[14]['text'] = '\u00A2';  // yen -> cent
  return data;
};


/**
 * Gets European More keyset characters.
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.more.keyEUMoreCharacters =
    function() {
  // Copy UK more characters.
  var data = i18n.input.chrome.inputview.content.compact.more.
      keyUKMoreCharacters();
  data[11]['text'] = '\u00A3';  // euro -> pound
  return data;
};


/**
 * Gets Pinyin More keyset characters.
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.more.keyPinyinMoreCharacters =
    function() {
  var data = i18n.input.chrome.inputview.content.compact.more.
      keyNAMoreCharacters();
  data[0]['text'] = '\uff5e';
  data[15]['text'] = '\u00B0';
  data[15]['moreKeys'] = {
        'characters': ['\u2032', '\u2033']};
  data[16]['text'] = '\u300e';
  data[16]['moreKeys'] = undefined;
  data[17]['text'] = '\u300f';
  data[17]['moreKeys'] = undefined;
  data[18]['text'] = '\uff5b';
  data[19]['text'] = '\uff5d';
  data[22]['text'] = '\uff0f';
  data[22]['moreKeys'] = undefined;
  data[27]['text'] = '\uff3b';
  data[28]['text'] = '\uff3d';
  data[29]['text'] = '\u3010';
  data[29]['moreKeys'] = undefined;
  data[30]['text'] = '\u3011';
  data[30]['moreKeys'] = undefined;
  data[33]['text'] = '\u300a';
  data[33]['moreKeys'] = {
        'characters': ['\u2039', '\u2264', '\u00AB']};
  data[35]['text'] = '\u300b';
  data[35]['moreKeys'] = {
        'characters': ['\u203A', '\u2265', '\u00BB']};
  data[37]['text'] = '\uff0c';
  data[38]['text'] = '\u3002';
  return data;
};

});  // goog.scope
