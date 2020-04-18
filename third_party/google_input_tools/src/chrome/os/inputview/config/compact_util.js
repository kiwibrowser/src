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
goog.provide('i18n.input.chrome.inputview.content.compact.util');
goog.provide('i18n.input.chrome.inputview.content.compact.util.CompactKeysetSpec');

goog.require('goog.object');
goog.require('i18n.input.chrome.ElementType');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.SpecNodeName');
goog.require('i18n.input.chrome.inputview.content.Constants');

goog.scope(function() {
var util = i18n.input.chrome.inputview.content.compact.util;
var NON_LETTER_KEYS =
    i18n.input.chrome.inputview.content.Constants.NON_LETTER_KEYS;
var Css = i18n.input.chrome.inputview.Css;
var SpecNodeName = i18n.input.chrome.inputview.SpecNodeName;


/**
 * The compact layout keyset type.
 *
 * @enum {string}
 */
util.CompactKeysetSpec = {
  ID: 'id',
  LAYOUT: 'layout',
  DATA: 'data'
};


/**
 * @desc The name of the layout providing numbers from 0-9 and common
 *     symbols.
 */
util.MSG_NUMBER_AND_SYMBOL =
    goog.getMsg('number and symbol layout');


/**
 * @desc The name of the layout providing more symbols.
 */
util.MSG_MORE_SYMBOLS =
    goog.getMsg('more symbols layout');


/**
 * @desc The name of the main layout.
 */
util.MSG_MAIN_LAYOUT = goog.getMsg('main layout');


/**
 * @desc The name of the english main layout.
 */
util.MSG_ENG_MAIN_LAYOUT = goog.getMsg('english main layout');


/**
 * @desc The name of the english layout providing numbers from 0-9 and common.
 */
util.MSG_ENG_MORE_SYMBOLS =
    goog.getMsg('english more symbols layout');


/**
 * @desc The name of the english layout providing more symbols.
 */
util.MSG_ENG_NUMBER_AND_SYMBOL =
    goog.getMsg('english number and symbol layout');


/**
 * Creates the compact key data.
 *
 * @param {!Object} keysetSpec The keyset spec.
 * @param {string} viewIdPrefix The prefix of the view.
 * @param {string} keyIdPrefix The prefix of the key.
 * @return {!Object} The key data.
 */
util.createCompactData = function(keysetSpec, viewIdPrefix, keyIdPrefix) {
  var keyList = [];
  var mapping = {};
  var keysetSpecNode = util.CompactKeysetSpec;
  for (var i = 0; i < keysetSpec[keysetSpecNode.DATA].length; i++) {
    var keySpec = keysetSpec[keysetSpecNode.DATA][i];
    if (keySpec == NON_LETTER_KEYS.MENU) {
      keySpec[SpecNodeName.TO_KEYSET] =
          keysetSpec[keysetSpecNode.ID].split('.')[0];
    }
    var id = keySpec[SpecNodeName.ID] ?
        keySpec[SpecNodeName.ID] :
        keyIdPrefix + i;
    var key = util.createCompactKey(id, keySpec);
    keyList.push(key);
    mapping[key['spec'][SpecNodeName.ID]] = viewIdPrefix + i;
  }

  var compactKeyData = {};
  compactKeyData[SpecNodeName.KEY_LIST] = keyList;
  compactKeyData[SpecNodeName.MAPPING] = mapping;
  compactKeyData[SpecNodeName.LAYOUT] = keysetSpec[keysetSpecNode.LAYOUT];
  return compactKeyData;
};


/**
 * Creates a key in the compact keyboard.
 *
 * @param {string} id The id.
 * @param {!Object} keySpec The specification.
 * @return {!Object} The compact key.
 */
util.createCompactKey = function(id, keySpec) {
  var spec = keySpec;
  spec[SpecNodeName.ID] = id;
  if (!spec[SpecNodeName.TYPE]) {
    spec[SpecNodeName.TYPE] =
        i18n.input.chrome.ElementType.COMPACT_KEY;
  }

  var newSpec = {};
  for (var key in spec) {
    newSpec[key] = spec[key];
  }

  return {
    'spec': newSpec
  };
};


/**
 * Customize the switcher keys in key characters.
 *
 * @param {!Array.<!Object>} keyCharacters The key characters.
 * @param {!Array.<!Object>} switcherKeys The customized switcher keys.
 */
util.customizeSwitchers = function(keyCharacters, switcherKeys) {
  var switcherKeyIndex = 0;
  for (var i = 0; i < keyCharacters.length; i++) {
    if (keyCharacters[i] == NON_LETTER_KEYS.SWITCHER) {
      if (switcherKeyIndex >= switcherKeys.length) {
        console.error('The number of switcher key spec is less than' +
            ' the number of switcher keys in the keyset.');
        return;
      }

      // Merge the switcher key and key character specifications.
      var newSpec = {};
      goog.object.extend(newSpec, switcherKeys[switcherKeyIndex]);
      goog.object.extend(newSpec, keyCharacters[i]);

      // Assign the merged specification to the key and
      // move on to the next switcher key.
      keyCharacters[i] = newSpec;
      switcherKeyIndex++;
    }
  }
  if (switcherKeyIndex < switcherKeys.length) {
    console.error('The number of switcher key spec is more than' +
        ' the number of switcher keys in the keyset.');
  }
};


/**
 * Attaches a keyset to the spacebar.
 * This is used to switch to the letter keyset when space is entered
 * after a symbol.
 *
 * @param {!Array.<!Object>} keyCharacters The key characters.
 * @param {string} letterKeysetId The ID of the letter keyset.
 * @private
 */
util.addKeysetToSpacebar_ = function(keyCharacters, letterKeysetId) {
  for (var i = 0; i < keyCharacters.length; i++) {
    if (keyCharacters[i] == NON_LETTER_KEYS.SPACE) {
      keyCharacters[i][SpecNodeName.TO_KEYSET] = letterKeysetId;
      break;
    }
  }
};


/**
 * Generates letter, symbol and more compact keysets, and loads them.
 *
 * @param {!Object} letterKeysetSpec The spec of letter keyset.
 * @param {!Object} symbolKeysetSpec The spec of symbol keyset.
 * @param {!Object} moreKeysetSpec The spec of more keyset.
 * @param {!function(!Object): void} onLoaded The function to call once keyset
 *     data is ready.
 */
util.generateCompactKeyboard =
    function(letterKeysetSpec, symbolKeysetSpec, moreKeysetSpec, onLoaded) {
  // Creates the switcher key specifications.
  var keysetSpecNode = util.CompactKeysetSpec;

  var lettersSwitcherKey = {};
  lettersSwitcherKey[SpecNodeName.NAME] = 'abc';
  lettersSwitcherKey[SpecNodeName.TO_KEYSET] =
      letterKeysetSpec[keysetSpecNode.ID];
  lettersSwitcherKey[SpecNodeName.TO_KEYSET_NAME] = util.MSG_MAIN_LAYOUT;

  var symbolsSwitcherKey = {};
  symbolsSwitcherKey[SpecNodeName.NAME] = '?123';
  symbolsSwitcherKey[SpecNodeName.TO_KEYSET] =
      symbolKeysetSpec[keysetSpecNode.ID];
  symbolsSwitcherKey[SpecNodeName.TO_KEYSET_NAME] = util.MSG_NUMBER_AND_SYMBOL;

  var moreSwitcherKey = {};
  moreSwitcherKey[SpecNodeName.NAME] = '~[<';
  moreSwitcherKey[SpecNodeName.TO_KEYSET] = moreKeysetSpec[keysetSpecNode.ID];
  moreSwitcherKey[SpecNodeName.TO_KEYSET_NAME] = util.MSG_MORE_SYMBOLS;

  // Creates compact qwerty keyset.
  util.customizeSwitchers(
      letterKeysetSpec[keysetSpecNode.DATA],
      [symbolsSwitcherKey]);
  var data = util.createCompactData(
      letterKeysetSpec, 'compactkbd-k-', 'compactkbd-k-key-');
  data[SpecNodeName.ID] = letterKeysetSpec[keysetSpecNode.ID];
  data[SpecNodeName.SHOW_MENU_KEY] = true;
  onLoaded(data);

  // Creates compact symbol keyset.
  util.customizeSwitchers(
      symbolKeysetSpec[keysetSpecNode.DATA],
      [moreSwitcherKey, moreSwitcherKey, lettersSwitcherKey]);
  util.addKeysetToSpacebar_(
      symbolKeysetSpec[keysetSpecNode.DATA],
      letterKeysetSpec[keysetSpecNode.ID]);
  data = util.createCompactData(
      symbolKeysetSpec, 'compactkbd-k-', 'compactkbd-k-key-');
  data[SpecNodeName.ID] = symbolKeysetSpec[keysetSpecNode.ID];
  data[SpecNodeName.SHOW_MENU_KEY] = false;
  data[SpecNodeName.NO_SHIFT] = true;
  onLoaded(data);

  // Creates compact more keyset.
  util.customizeSwitchers(
      moreKeysetSpec[keysetSpecNode.DATA],
      [symbolsSwitcherKey, symbolsSwitcherKey, lettersSwitcherKey]);
  data = util.createCompactData(moreKeysetSpec, 'compactkbd-k-',
      'compactkbd-k-key-');
  data[SpecNodeName.ID] = moreKeysetSpec[keysetSpecNode.ID];
  data[SpecNodeName.SHOW_MENU_KEY] = false;
  data[SpecNodeName.NO_SHIFT] = true;
  onLoaded(data);
};


/**
 * Generates letter, symbol and more compact keysets for
 *     pinyin's chinese and english mode and load them.
 *
 * @param {!Object} letterKeysetSpec The spec of letter keyset.
 * @param {!Object} engKeysetSpec The english spec of letter keyset
 * @param {!Object} symbolKeysetSpec The spec of symbol keyset.
 * @param {!Object} engSymbolKeysetSpec The spec of engish symbol keyset.
 * @param {!Object} moreKeysetSpec The spec of more keyset.
 * @param {!Object} engMoreKeysetSpec The spec of english more keyset.
 * @param {!function(!Object): void} onLoaded The function to call once a keyset
 *     data is ready.
 */
util.generatePinyinCompactKeyboard = function(letterKeysetSpec, engKeysetSpec,
    symbolKeysetSpec, engSymbolKeysetSpec,
    moreKeysetSpec, engMoreKeysetSpec, onLoaded) {
  // Creates the switcher key specifications.
  var keysetSpecNode = util.CompactKeysetSpec;

  var lettersSwitcherKey = {};
  lettersSwitcherKey[SpecNodeName.NAME] = 'abc';
  lettersSwitcherKey[SpecNodeName.TO_KEYSET] =
      letterKeysetSpec[keysetSpecNode.ID];
  lettersSwitcherKey[SpecNodeName.TO_KEYSET_NAME] = util.MSG_MAIN_LAYOUT;

  var symbolsSwitcherKey = {};
  symbolsSwitcherKey[SpecNodeName.NAME] = '?123';
  symbolsSwitcherKey[SpecNodeName.TO_KEYSET] =
      symbolKeysetSpec[keysetSpecNode.ID];
  symbolsSwitcherKey[SpecNodeName.TO_KEYSET_NAME] = util.MSG_NUMBER_AND_SYMBOL;

  var moreSwitcherKey = {};
  moreSwitcherKey[SpecNodeName.NAME] = '~[<';
  moreSwitcherKey[SpecNodeName.TO_KEYSET] = moreKeysetSpec[keysetSpecNode.ID];
  moreSwitcherKey[SpecNodeName.TO_KEYSET_NAME] = util.MSG_MORE_SYMBOLS;

  var engLettersSwitcherKey = {};
  engLettersSwitcherKey[SpecNodeName.NAME] = 'abc';
  engLettersSwitcherKey[SpecNodeName.TO_KEYSET] =
      engKeysetSpec[keysetSpecNode.ID];
  engLettersSwitcherKey[SpecNodeName.TO_KEYSET_NAME] = util.MSG_ENG_MAIN_LAYOUT;

  var engSymbolsSwitcherKey = {};
  engSymbolsSwitcherKey[SpecNodeName.NAME] = '?123';
  engSymbolsSwitcherKey[SpecNodeName.TO_KEYSET] =
      engSymbolKeysetSpec[keysetSpecNode.ID];
  engSymbolsSwitcherKey[SpecNodeName.TO_KEYSET_NAME] =
      util.MSG_ENG_NUMBER_AND_SYMBOL;

  var engMoreSwitcherKey = {};
  engMoreSwitcherKey[SpecNodeName.NAME] = '~[<';
  engMoreSwitcherKey[SpecNodeName.TO_KEYSET] =
      engMoreKeysetSpec[keysetSpecNode.ID];
  engMoreSwitcherKey[SpecNodeName.TO_KEYSET_NAME] = util.MSG_ENG_MORE_SYMBOLS;

  var lettersSwitcherKeyWithIcon = {};
  lettersSwitcherKeyWithIcon[SpecNodeName.TO_KEYSET] =
      letterKeysetSpec[keysetSpecNode.ID];
  lettersSwitcherKeyWithIcon[SpecNodeName.TO_KEYSET_NAME] =
      util.MSG_MAIN_LAYOUT;
  lettersSwitcherKeyWithIcon[SpecNodeName.ICON_CSS_CLASS] =
      Css.SWITCHER_ENGLISH;

  var engSwitcherKey = {};
  engSwitcherKey[SpecNodeName.TO_KEYSET] = engKeysetSpec[keysetSpecNode.ID];
  engSwitcherKey[SpecNodeName.TO_KEYSET_NAME] = util.MSG_ENG_MAIN_LAYOUT;
  engSwitcherKey[SpecNodeName.ICON_CSS_CLASS] = Css.SWITCHER_CHINESE;

  // Creates compact qwerty keyset for pinyin.
  util.customizeSwitchers(
      letterKeysetSpec[keysetSpecNode.DATA],
      [symbolsSwitcherKey, engSwitcherKey]);
  var data = util.createCompactData(
      letterKeysetSpec, 'compactkbd-k-', 'compactkbd-k-key-');
  data[SpecNodeName.ID] = letterKeysetSpec[keysetSpecNode.ID];
  data[SpecNodeName.SHOW_MENU_KEY] = true;
  onLoaded(data);

  // Creates the compact qwerty keyset for pinyin's English mode.
  util.customizeSwitchers(
      engKeysetSpec[keysetSpecNode.DATA],
      [engSymbolsSwitcherKey, lettersSwitcherKeyWithIcon]);
  data = util.createCompactData(
      engKeysetSpec, 'compactkbd-k-', 'compactkbd-k-key-');
  data[SpecNodeName.ID] = engKeysetSpec[keysetSpecNode.ID];
  data[SpecNodeName.SHOW_MENU_KEY] = true;
  onLoaded(data);

  // Creates compact symbol keyset for pinyin.
  util.customizeSwitchers(
      symbolKeysetSpec[keysetSpecNode.DATA],
      [moreSwitcherKey, moreSwitcherKey, lettersSwitcherKey]);
  data = util.createCompactData(
      symbolKeysetSpec, 'compactkbd-k-', 'compactkbd-k-key-');
  data[SpecNodeName.ID] = symbolKeysetSpec[keysetSpecNode.ID];
  data[SpecNodeName.SHOW_MENU_KEY] = false;
  data[SpecNodeName.NO_SHIFT] = true;
  onLoaded(data);

  // Creates compact symbol keyset for English mode.
  util.customizeSwitchers(
      engSymbolKeysetSpec[keysetSpecNode.DATA],
      [engMoreSwitcherKey, engMoreSwitcherKey, engLettersSwitcherKey]);
  data = util.createCompactData(
      engSymbolKeysetSpec, 'compactkbd-k-', 'compactkbd-k-key-');
  data[SpecNodeName.ID] = engSymbolKeysetSpec[keysetSpecNode.ID];
  data[SpecNodeName.SHOW_MENU_KEY] = false;
  data[SpecNodeName.NO_SHIFT] = true;
  onLoaded(data);

  // Creates compact more keyset for pinyin.
  util.customizeSwitchers(
      moreKeysetSpec[keysetSpecNode.DATA],
      [symbolsSwitcherKey, symbolsSwitcherKey, lettersSwitcherKey]);
  data = util.createCompactData(moreKeysetSpec, 'compactkbd-k-',
      'compactkbd-k-key-');
  data[SpecNodeName.ID] = moreKeysetSpec[keysetSpecNode.ID];
  data[SpecNodeName.SHOW_MENU_KEY] = false;
  data[SpecNodeName.NO_SHIFT] = true;
  onLoaded(data);

  // Creates the compact more keyset of english mode.
  util.customizeSwitchers(
      engMoreKeysetSpec[keysetSpecNode.DATA],
      [engSymbolsSwitcherKey, engSymbolsSwitcherKey, engLettersSwitcherKey]);
  data = util.createCompactData(engMoreKeysetSpec, 'compactkbd-k-',
      'compactkbd-k-key-');
  data[SpecNodeName.ID] = engMoreKeysetSpec[keysetSpecNode.ID];
  data[SpecNodeName.SHOW_MENU_KEY] = false;
  data[SpecNodeName.NO_SHIFT] = true;
  onLoaded(data);
};
});  // goog.scope
