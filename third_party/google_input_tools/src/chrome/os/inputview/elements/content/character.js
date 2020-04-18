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
goog.provide('i18n.input.chrome.inputview.elements.content.Character');

goog.require('goog.dom');
goog.require('goog.dom.classlist');
goog.require('goog.style');
goog.require('i18n.input.chrome.ElementType');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.elements.Element');
goog.require('i18n.input.chrome.inputview.util');



goog.scope(function() {



/**
 * The letter in the soft key.
 *
 * @param {string} id The id.
 * @param {!i18n.input.chrome.inputview.elements.content.CharacterModel} model
 *     The character model.
 * @param {boolean} isRTL Whether the character is shown in a RTL layout.
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.Element}
 */
i18n.input.chrome.inputview.elements.content.Character = function(
    id, model, isRTL) {
  goog.base(this, id, i18n.input.chrome.ElementType.
      CHARACTER);

  /**
   * The model.
   *
   * @type {!i18n.input.chrome.inputview.elements.content.CharacterModel}
   * @private
   */
  this.characterModel_ = model;

  /**
   * Whether the character is shown in a RTL layout.
   *
   * @type {boolean}
   * @private
   */
  this.isRTL_ = isRTL;
};
goog.inherits(i18n.input.chrome.inputview.elements.content.Character,
    i18n.input.chrome.inputview.elements.Element);
var Character = i18n.input.chrome.inputview.elements.content.Character;


/**
 * The padding.
 *
 * @type {number}
 * @private
 */
Character.PADDING_ = 2;


/** @override */
Character.prototype.createDom = function() {
  goog.base(this, 'createDom');

  var elem = this.getElement();
  var dom = this.getDomHelper();
  this.getElement()['view'] = null;
  var character = this.characterModel_.getContent();
  dom.appendChild(elem, dom.createTextNode(character));
  goog.dom.classlist.add(elem, i18n.input.chrome.inputview.Css.CHARACTER);
  if (/[0-9]/.test(character)) {
    // Digits 0-9 looks bigger than it should be, so decrease the font-size for
    // them.
    goog.dom.classlist.add(elem,
        i18n.input.chrome.inputview.Css.DIGIT_CHARACTER);
  }
  elem.style.direction = this.isRTL_ ? 'rtl' : 'ltr';
};


/**
 * Reposition the character.
 *
 * @private
 */
Character.prototype.reposition_ = function() {
  var width = this.width;
  var height = this.height;
  var size = goog.style.getSize(this.getElement());
  var paddingVertical;
  var paddingHorizontal;
  if (this.characterModel_.isHorizontalAlignCenter()) {
    paddingHorizontal = Math.floor((width - size.width) / 2);
  } else {
    paddingHorizontal = Character.PADDING_;
  }
  if (this.characterModel_.isVerticalAlignCenter()) {
    paddingVertical = Math.floor((height - size.height) / 2);
  } else {
    paddingVertical = Character.PADDING_;
  }
  var attributes = this.characterModel_.getPositionAttribute();
  var elem = this.getElement();
  elem.style[attributes[0]] = paddingVertical + 'px';
  elem.style[attributes[1]] = paddingHorizontal + 'px';
};


/**
 * Highlights the letter or not.
 */
Character.prototype.highlight = function() {
  if (this.characterModel_.isHighlighted()) {
    goog.dom.classlist.add(this.getElement(),
        i18n.input.chrome.inputview.Css.CHARACTER_HIGHLIGHT);
  } else {
    goog.dom.classlist.remove(this.getElement(),
        i18n.input.chrome.inputview.Css.CHARACTER_HIGHLIGHT);
  }
};


/**
 * Updates the content.
 */
Character.prototype.updateContent = function() {
  var ch = this.characterModel_.getContent();
  goog.dom.setTextContent(this.getElement(),
      i18n.input.chrome.inputview.util.getVisibleCharacter(ch));
};


/** @override */
Character.prototype.setVisible = function(visibility) {
  this.getElement().style.display = visibility ? 'inline-block' : 'none';
};


/** @override */
Character.prototype.resize = function(width, height) {
  goog.base(this, 'resize', width, height);

  this.update();
};


/** @override */
Character.prototype.update = function() {
  var visible = this.characterModel_.isVisible();
  this.setVisible(visible);
  if (visible) {
    this.highlight();
    this.updateContent();
    this.reposition_();
  }
};


/**
 * Gets the letter.
 *
 * @return {string} The letter.
 */
Character.prototype.getContent = function() {
  return this.characterModel_.getContent();
};


/** @override */
Character.prototype.isVisible = function() {
  return this.characterModel_.isVisible();
};


/**
 * True if this character is highlighted.
 *
 * @return {boolean} True if this character is highlighted.
 */
Character.prototype.isHighlighted = function() {
  return this.characterModel_.isHighlighted();
};


});  // goog.scope
