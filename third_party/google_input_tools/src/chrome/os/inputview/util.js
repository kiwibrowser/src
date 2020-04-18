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
goog.provide('i18n.input.chrome.inputview.util');

goog.require('goog.string');
goog.require('goog.style');


goog.scope(function() {
var util = i18n.input.chrome.inputview.util;


/**
 * The mapping between the real character and its replacement for display.
 *
 * @type {!Object.<string, string>}
 */
util.DISPLAY_MAPPING = {
  '\u0300' : '\u0060',
  '\u0301' : '\u00B4',
  '\u0302' : '\u02C6',
  '\u0303' : '\u02DC',
  '\u0304' : '\u02c9',
  '\u0305' : '\u00AF',
  '\u0306' : '\u02D8',
  '\u0307' : '\u02D9',
  '\u0308' : '\u00a8',
  '\u0309' : '\u02C0',
  '\u030A' : '\u02DA',
  '\u030B' : '\u02DD',
  '\u030C' : '\u02C7',
  '\u030D' : '\u02C8',
  '\u030E' : '\u0022',
  '\u0327' : '\u00B8',
  '\u0328' : '\u02DB',
  '\u0345' : '\u037A',
  '\u030F' : '\u030F\u0020',
  '\u031B' : '\u031B\u0020',
  '\u0323' : '\u0323\u0020'
};


/**
 * Special lower to upper case mapping.
 *
 * @private {!Object.<string, string>}
 */
util.CASE_LOWER_TO_UPPER_MAPPING_ = {
  '\u00b5': '\u00b5',
  // Korean characters.
  '\u3142': '\u3143',
  '\u3148': '\u3149',
  '\u3137': '\u3138',
  '\u3131': '\u3132',
  '\u3145': '\u3146',
  '\u1162': '\u1164',
  '\u1166': '\u1168'
};


/**
 * Special upper to lower case mapping.
 *
 * @private {!Object.<string, string>}
 */
util.CASE_UPPER_TO_LOWER_MAPPING_ = {
  '\u0049': '\u0131'
};


/**
 * The keysets using US keyboard layouts.
 *
 * @type {!Array.<string>}
 */
util.KEYSETS_USE_US = [
  'array',
  'cangjie',
  'dayi',
  'jp_us',
  'pinyin-zh-CN',
  'pinyin-zh-TW',
  'quick',
  't13n',
  'wubi',
  'zhuyin.us',
  'ko.set2.us',
  'ko.set390.us',
  'ko.set3final.us',
  'ko.set3sun.us',
  'ko.romaja.us',
  'ko.ahn.us',
  'ko.set2y.us',
  'ko.set3yet.us',
  'ko.set32.us'
];


/**
 * The keysets that have en switcher key.
 *
 * @type {!Array.<string>}
 */
util.KEYSETS_HAVE_EN_SWTICHER = [
  // When other keysets that use us add the enswitcher key,
  // should move them to this array.
  'array',
  'cangjie',
  'dayi',
  'pinyin-zh-CN',
  'pinyin-zh-TW',
  'quick',
  'wubi',
  'zhuyin',
  'zhuyin.us'
];


/**
 * The keysets that can switch with US keyboard layouts.
 *
 * @type {!Array.<string>}
 */
util.KEYSETS_SWITCH_WITH_US = [
  'zhuyin',
  'ko.set2',
  'ko.set390',
  'ko.set3final',
  'ko.set3sun',
  'ko.romaja',
  'ko.ahn',
  'ko.set2y',
  'ko.set3yet',
  'ko.set32'
];


/**
 * The keysets that have compact keyset.
 *
 * @type {!Array.<string>}
 */
util.KEYSETS_HAVE_COMPACT = [
  'ca',
  'ca-eng',
  'de',
  'dk',
  'fi',
  'fr',
  'gb-extd',
  'ie',
  'is',
  'nl',
  'no',
  'pinyin-zh-CN',
  'se',
  'us',
  'zhuyin'
];


/**
 * A regular expression for the end of a sentence.
 *
 * @private {!RegExp}
 */
util.END_SENTENCE_REGEX_ = /[\.\?!] +$/;


/**
 * The regex of characters support dead key.
 *
 * @type {!RegExp}
 * @private
 */
util.REGEX_CHARACTER_SUPPORT_DEADKEY_ =
    /^[a-zA-ZæÆœŒΑΕΗΙΟΥΩαεηιυοωϒ]+$/;


/**
 * The regex of characters supported in language module.
 *
 * @type {!RegExp}
 */
util.REGEX_LANGUAGE_MODEL_CHARACTERS =
    /(?=[^\u00d7\u00f7])[a-z\-\'\u00c0-\u017F]/i;


/**
 * Splits a value to pieces according to the weights.
 *
 * @param {!Array.<number>} weightArray The weight array.
 * @param {number} totalValue The total value.
 * @return {!Array.<number>} The splitted values.
 */
util.splitValue = function(weightArray, totalValue) {
  if (weightArray.length == 0) {
    return [];
  }

  if (weightArray.length == 1) {
    return [totalValue];
  }

  var totalWeight = 0;
  for (var i = 0; i < weightArray.length; i++) {
    totalWeight += weightArray[i];
  }
  var tmp = totalValue / totalWeight;
  var values = [];
  var totalFlooredValue = 0;
  var diffs = [];
  for (var i = 0; i < weightArray.length; i++) {
    var result = weightArray[i] * tmp;
    values.push(result);
    diffs.push(result - Math.floor(result));
    totalFlooredValue += Math.floor(result);
  }
  var diff = totalValue - totalFlooredValue;

  // Distributes the rest pixels to values who lose most.
  for (var i = 0; i < diff; i++) {
    var max = 0;
    var index = 0;
    for (var j = 0; j < diffs.length; j++) {
      if (diffs[j] > max) {
        max = diffs[j];
        index = j;
      }
    }
    values[index] += 1;
    diffs[index] = 0;
  }
  for (var i = 0; i < values.length; i++) {
    values[i] = Math.floor(values[i]);
  }
  return values;
};


/**
 * Gets the value of a property.
 *
 * @param {Element} elem The element.
 * @param {string} property The property name.
 * @return {number} The value.
 */
util.getPropertyValue = function(elem, property) {
  var value = goog.style.getComputedStyle(elem, property);
  if (value) {
    return parseInt(value.replace('px', ''), 10);
  }
  return 0;
};


/**
 * To upper case.
 *
 * @param {string} character The character.
 * @return {string} The uppercase of the character.
 */
util.toUpper = function(character) {
  var upper = util.CASE_LOWER_TO_UPPER_MAPPING_[character];
  if (upper) {
    return upper;
  }
  return character.toUpperCase();
};


/**
 * To lower case.
 *
 * @param {string} character The character.
 * @return {string} The lower case of the character.
 */
util.toLower = function(character) {
  var lower = util.CASE_UPPER_TO_LOWER_MAPPING_[character];
  if (lower) {
    return lower;
  }
  return character.toLowerCase();
};


/**
 * Is this character trigger commit.
 *
 * @param {string} character The character.
 * @return {boolean} True to trigger commit.
 */
util.isCommitCharacter = function(character) {
  if (util.DISPLAY_MAPPING[character] ||
      util.REGEX_LANGUAGE_MODEL_CHARACTERS.test(
          character)) {
    return false;
  }

  return true;
};


/**
 * Some unicode character can't be shown in the web page, use a replacement
 *     instead.
 *
 * @param {string} invisibleCharacter The character can't be shown.
 * @return {string} The replacement.
 */
util.getVisibleCharacter = function(invisibleCharacter) {
  var map = util.DISPLAY_MAPPING;
  if (map[invisibleCharacter]) {
    return map[invisibleCharacter];
  }
  // For non-spacing marks (e.g. \u05b1), ChromeOS cannot display it correctly
  // until there is a character before it to combine with.
  if (/[\u0591-\u05cf]/.test(invisibleCharacter)) {
    return '\u00a0' + invisibleCharacter;
  }
  return invisibleCharacter;
};


/**
 * Whether this is a letter key.
 *
 * @param {!Array.<string>} characters The characters.
 * @return {boolean} True if this is a letter key.
 */
util.isLetterKey = function(characters) {
  if (characters[1] == util.toUpper(
      characters[0]) || characters[1] == util.
          toLower(characters[0])) {
    return true;
  }
  return false;
};


/**
 * True if this character supports dead key combination.
 *
 * @param {string} character The character.
 * @return {boolean} True if supports the dead key combination.
 */
util.supportDeadKey = function(character) {
  return util.REGEX_CHARACTER_SUPPORT_DEADKEY_.
      test(character);
};


/**
 * True if we need to do the auto-capitalize.
 *
 * @param {string} text .
 * @return {boolean} .
 */
util.needAutoCap = function(text) {
  if (goog.string.isEmptyOrWhitespace(text)) {
    return false;
  } else {
    return util.END_SENTENCE_REGEX_.test(text);
  }
};


/**
 * Returns the configuration file name from the keyboard code.
 *
 * @param {string} keyboardCode The keyboard code.
 * @return {string} The config file name which contains the keyset.
 */
util.getConfigName = function(keyboardCode) {
  // Strips out all the suffixes in the keyboard code.
  return keyboardCode.replace(/\..*$/, '');
};
});  // goog.scope
