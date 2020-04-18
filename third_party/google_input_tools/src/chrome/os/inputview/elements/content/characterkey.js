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
goog.provide('i18n.input.chrome.inputview.elements.content.CharacterKey');

goog.require('goog.a11y.aria');
goog.require('goog.a11y.aria.State');
goog.require('goog.array');
goog.require('i18n.input.chrome.ElementType');
goog.require('i18n.input.chrome.inputview.StateType');
goog.require('i18n.input.chrome.inputview.SwipeDirection');
goog.require('i18n.input.chrome.inputview.elements.content.Character');
goog.require('i18n.input.chrome.inputview.elements.content.CharacterModel');
goog.require('i18n.input.chrome.inputview.elements.content.SoftKey');



goog.scope(function() {
var CharacterModel = i18n.input.chrome.inputview.elements.content.
    CharacterModel;
var Character = i18n.input.chrome.inputview.elements.content.Character;
var StateType = i18n.input.chrome.inputview.StateType;



/**
 * The class for a character key, it would be symbol or letter key which is
 * different than modifier key and functional key.
 *
 * @param {string} id The id.
 * @param {number} keyCode The key code.
 * @param {!Array.<string>} characters The characters.
 * @param {boolean} isLetterKey True if this is a letter key.
 * @param {boolean} hasAltGrCharacterInTheKeyset True if there is altgr
 *     character in the keyset.
 * @param {boolean} alwaysRenderAltGrCharacter True if always renders the altgr
 *     character.
 * @param {!i18n.input.chrome.inputview.StateManager} stateManager The state
 *     manager.
 * @param {boolean} isRTL Whether the key shows characters in a RTL layout.
 * @param {boolean} enableShiftRendering Whether renders two letter vertically,
 *     it means show shift letter when in letter state, shows default letter
 *     when in shift state, same as the altgr state.
 * @param {goog.events.EventTarget=} opt_eventTarget The event target.
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.content.SoftKey}
 */
i18n.input.chrome.inputview.elements.content.CharacterKey = function(id,
    keyCode, characters, isLetterKey, hasAltGrCharacterInTheKeyset,
    alwaysRenderAltGrCharacter, stateManager, isRTL,
    enableShiftRendering, opt_eventTarget) {
  goog.base(this, id, i18n.input.chrome.ElementType.
      CHARACTER_KEY, opt_eventTarget);

  /**
   * The key code for sending key events.
   *
   * @type {number}
   */
  this.keyCode = keyCode;

  /**
   * The content to shown in the soft key.
   *
   * @type {!Array.<string>}
   */
  this.characters = characters;

  /**
   * True if this is a letter key.
   *
   * @type {boolean}
   */
  this.isLetterKey = isLetterKey;

  /**
   * True if there is altgr character in the keyset.
   *
   * @type {boolean}
   * @private
   */
  this.hasAltGrCharacterInTheKeyset_ = hasAltGrCharacterInTheKeyset;

  /**
   * The state manager.
   *
   * @type {!i18n.input.chrome.inputview.StateManager}
   * @private
   */
  this.stateManager_ = stateManager;

  /**
   * Whether the key shows characters in a RTL layout.
   *
   * @type {boolean}
   * @private
   */
  this.isRTL_ = isRTL;

  /**
   * Whether to always renders the altgr character..
   *
   * @type {boolean}
   * @private
   */
  this.alwaysRenderAltGrCharacter_ = alwaysRenderAltGrCharacter;

  /** @private {boolean} */
  this.enableShiftRendering_ = enableShiftRendering;

  this.pointerConfig.longPressWithPointerUp = true;
  this.pointerConfig.longPressDelay = 500;
};
goog.inherits(i18n.input.chrome.inputview.elements.content.CharacterKey,
    i18n.input.chrome.inputview.elements.content.SoftKey);
var CharacterKey = i18n.input.chrome.inputview.elements.content.CharacterKey;


/**
 * The flickerred character.
 *
 * @type {string}
 */
CharacterKey.prototype.flickerredCharacter = '';


/**
 * The state map.
 *
 * @type {!Array.<number>}
 * @private
 */
CharacterKey.STATE_LIST_ = [
  StateType.DEFAULT,
  StateType.SHIFT,
  StateType.ALTGR,
  StateType.ALTGR | StateType.SHIFT
];


/** @override */
CharacterKey.prototype.createDom = function() {
  goog.base(this, 'createDom');

  for (var i = 0; i < CharacterKey.STATE_LIST_.length; i++) {
    var ch = this.characters.length > i ? this.characters[i] : '';
    if (ch && ch != '\x00') {
      var model = new CharacterModel(ch, this.isLetterKey,
          this.hasAltGrCharacterInTheKeyset_,
          this.alwaysRenderAltGrCharacter_,
          CharacterKey.STATE_LIST_[i],
          this.stateManager_,
          this.enableShiftRendering_,
          this.getCapslockCharacter_(i));
      var character = new Character(this.id + '-' + i, model, this.isRTL_);
      this.addChild(character, true);
    }
  }
};


/**
 * Gets the capslock character if have.
 *
 * @param {number} i .
 * @private
 * @return {string} .
 */
CharacterKey.prototype.getCapslockCharacter_ = function(i) {
  var capslockCharacterIndex = i + 4;
  if (this.characters.length > capslockCharacterIndex) {
    return this.characters[capslockCharacterIndex];
  }

  return '';
};


/** @override */
CharacterKey.prototype.resize = function(width,
    height) {
  goog.base(this, 'resize', width, height);

  for (var i = 0; i < this.getChildCount(); i++) {
    var child = /** @type {!i18n.input.chrome.inputview.elements.Element} */ (
        this.getChildAt(i));
    child.resize(this.availableWidth, this.availableHeight);
  }
};


/**
 * Gets the alternative characters.
 *
 * @return {!Array.<string>} The characters.
 */
CharacterKey.prototype.getAltCharacters =
    function() {
  var altCharacters = [];
  for (var i = 0; i < this.characters.length; i++) {
    var ch = this.characters[i];
    if (ch && ch != '\x00' && ch != this.getActiveCharacter()) {
      goog.array.insert(altCharacters, ch);
    }
  }
  goog.array.removeDuplicates(altCharacters);
  return altCharacters;
};


/**
 * The active letter.
 *
 * @return {string} The active letter.
 */
CharacterKey.prototype.getActiveCharacter = function() {
  if (this.flickerredCharacter) {
    return this.flickerredCharacter;
  }

  for (var i = 0; i < this.getChildCount(); i++) {
    var child = /** @type {!i18n.input.chrome.inputview.elements.content.
        Character} */ (this.getChildAt(i));
    if (child.isHighlighted()) {
      return child.getContent();
    }
  }
  return '';
};


/**
 * Gets the character by gesture direction.
 *
 * @param {boolean} upOrDown True if up, false if down.
 * @return {string} The character content.
 */
CharacterKey.prototype.getCharacterByGesture =
    function(upOrDown) {
  var hasAltGrState = this.stateManager_.hasState(StateType.ALTGR);
  var hasShiftState = this.stateManager_.hasState(StateType.SHIFT);

  if (upOrDown == hasShiftState) {
    // When shift is on, we only take swipe down, otherwise we only
    // take swipe up.
    return '';
  }

  // The index is based on the characters in order:
  // 0: Default
  // 1: Shift
  // 2: ALTGR
  // 3: SHIFT + ALTGR
  var index = 0;
  if (upOrDown && hasAltGrState) {
    index = 3;
  } else if (upOrDown && !hasAltGrState) {
    index = 1;
  } else if (!upOrDown && hasAltGrState) {
    index = 2;
  }

  var character = index >= this.getChildCount() ? null :
      /** @type {!i18n.input.chrome.inputview.elements.content.Character} */
      (this.getChildAt(index));
  if (character && character.isVisible()) {
    return character.getContent();
  }
  return '';
};


/** @override */
CharacterKey.prototype.update = function() {
  goog.base(this, 'update');

  this.pointerConfig.flickerDirection = this.stateManager_.hasState(
      StateType.SHIFT) ?
      i18n.input.chrome.inputview.SwipeDirection.DOWN :
      i18n.input.chrome.inputview.SwipeDirection.UP;

  var ariaLabel = this.getActiveCharacter();
  if (this.isLetterKey &&
      (this.stateManager_.hasState(StateType.SHIFT) ||
          this.stateManager_.hasState(StateType.CAPSLOCK))) {
    ariaLabel = 'cap ' + ariaLabel;
  }
  goog.a11y.aria.setState(/** @type {!Element} */ (this.getElement()),
      goog.a11y.aria.State.LABEL, ariaLabel);
};

});  // goog.scope
