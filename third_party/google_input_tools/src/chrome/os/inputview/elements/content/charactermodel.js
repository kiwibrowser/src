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
goog.provide('i18n.input.chrome.inputview.elements.content.CharacterModel');

goog.require('i18n.input.chrome.inputview.StateType');
goog.require('i18n.input.chrome.inputview.util');



goog.scope(function() {
var StateType = i18n.input.chrome.inputview.StateType;
var util = i18n.input.chrome.inputview.util;



/**
 * The character model.
 *
 * @param {string} character The character.
 * @param {boolean} belongToLetterKey True if this characters belongs to a
 *     letter key.
 * @param {boolean} hasAltGrCharacterInTheKeyset True if this kind of key has
 *     altgr character.
 * @param {boolean} alwaysRenderAltGrCharacter True if always renders the altgr
 *     character.
 * @param {number} stateType The state type for this character.
 * @param {!i18n.input.chrome.inputview.StateManager} stateManager The state
 *     manager.
 * @param {boolean} enableShiftRendering .
 * @param {string=} opt_capslockCharacter .
 * @constructor
 */
i18n.input.chrome.inputview.elements.content.CharacterModel = function(
    character, belongToLetterKey, hasAltGrCharacterInTheKeyset,
    alwaysRenderAltGrCharacter, stateType, stateManager, enableShiftRendering,
    opt_capslockCharacter) {

  /**
   * The character.
   *
   * @type {string}
   * @private
   */
  this.character_ = character;

  /**
   * The character for the capslock state.
   *
   * @private {string}
   */
  this.capslockCharacter_ = opt_capslockCharacter || '';

  /**
   * Whether this character is belong to a letter key.
   *
   * @type {boolean}
   * @private
   */
  this.belongToLetterKey_ = belongToLetterKey;

  /**
   * The state.
   *
   * @type {number}
   * @private
   */
  this.stateType_ = stateType;

  /**
   * The state manager.
   *
   * @type {!i18n.input.chrome.inputview.StateManager}
   * @private
   */
  this.stateManager_ = stateManager;

  /**
   * Whether to always renders the altgr character..
   *
   * @type {boolean}
   * @private
   */
  this.alwaysRenderAltGrCharacter_ = alwaysRenderAltGrCharacter;

  /**
   * True if this key set has altgr character.
   *
   * @type {boolean}
   * @private
   */
  this.hasAltGrCharacterInTheKeyset_ = hasAltGrCharacterInTheKeyset;

  /** @private {boolean} */
  this.enableShiftRendering_ = enableShiftRendering;
};
var CharacterModel = i18n.input.chrome.inputview.elements.content.
    CharacterModel;


/**
 * The alignment type.
 *
 * @enum {number}
 */
CharacterModel.AlignType = {
  CENTER: 0,
  CORNER: 1
};


/**
 * The position attributes.
 *
 * @type {!Array.<!Array.<string>>}
 * @private
 */
CharacterModel.CORNERS_ = [
  ['bottom', 'left'],
  ['top', 'left'],
  ['bottom', 'right'],
  ['top', 'right']
];


/**
 * True if this character is highlighed.
 *
 * @return {boolean} True if the character is highlighted.
 */
CharacterModel.prototype.isHighlighted = function() {
  var state = this.stateManager_.getState();
  state = state & (StateType.SHIFT | StateType.ALTGR);
  return this.stateType_ == state;
};


/**
 * True if this character is visible.
 *
 * @return {boolean} True if the character is visible.
 */
CharacterModel.prototype.isVisible = function() {
  var hasShift = this.stateManager_.hasState(StateType.SHIFT);
  var enableShiftLetter = !this.belongToLetterKey_ || hasShift;
  var enableDefaultLetter = !this.belongToLetterKey_ || !hasShift;
  enableShiftLetter = (this.enableShiftRendering_ &&
      !this.belongToLetterKey_) || hasShift;
  enableDefaultLetter = (this.enableShiftRendering_ &&
      !this.belongToLetterKey_) || !hasShift;
  if (this.stateType_ == StateType.DEFAULT) {
    return !this.stateManager_.hasState(StateType.ALTGR) && enableDefaultLetter;
  }
  if (this.stateType_ == StateType.SHIFT) {
    return !this.stateManager_.hasState(StateType.ALTGR) && enableShiftLetter;
  }
  if (this.stateType_ == StateType.ALTGR) {
    // AltGr character.
    return this.stateManager_.hasState(StateType.ALTGR) && enableDefaultLetter;
  }
  if (this.stateType_ == (StateType.ALTGR | StateType.SHIFT)) {
    // Shift + AltGr character.
    return this.stateManager_.hasState(StateType.ALTGR) && enableShiftLetter;
  }
  return false;
};


/**
 * Gets the reversed case character.
 *
 * @return {string} The reversed character
 * @private
 */
CharacterModel.prototype.toReversedCase_ = function() {
  var upper = util.toUpper(this.character_);
  var lower = util.toLower(this.character_);
  return (upper == this.character_) ? lower : upper;
};


/**
 * Gets the content of this character..
 *
 * @return {string} The content.
 */
CharacterModel.prototype.getContent = function() {
  if (this.stateManager_.hasState(StateType.CAPSLOCK)) {
    return this.capslockCharacter_ ? this.capslockCharacter_ :
        this.toReversedCase_();
  }

  return this.character_;
};


/**
 * True if align the character in the center horizontally.
 *
 * @return {boolean} True to align in the center.
 */
CharacterModel.prototype.isHorizontalAlignCenter = function() {
  if (this.stateType_ == StateType.DEFAULT ||
      this.stateType_ == StateType.SHIFT) {
    return !this.alwaysRenderAltGrCharacter_ ||
        !this.hasAltGrCharacterInTheKeyset_;
  }

  return true;
};


/**
 * True to align the character in the center vertically.
 *
 * @return {boolean} True to be in the center.
 */
CharacterModel.prototype.isVerticalAlignCenter = function() {
  if (this.stateType_ == StateType.DEFAULT ||
      this.stateType_ == StateType.SHIFT) {
    return this.belongToLetterKey_;
  }

  return false;
};


/**
 * Gets the attribute for position.
 *
 * @return {!Array.<string>} The attributes.
 */
CharacterModel.prototype.getPositionAttribute = function() {
  switch (this.stateType_) {
    case StateType.DEFAULT:
      return CharacterModel.CORNERS_[0];
    case StateType.SHIFT:
      return CharacterModel.CORNERS_[1];
    case StateType.ALTGR:
      return CharacterModel.CORNERS_[2];
    default:
      return CharacterModel.CORNERS_[3];
  }
};


});  // goog.scope

