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
goog.provide('i18n.input.chrome.inputview.elements.content.CompactKey');

goog.require('goog.a11y.aria');
goog.require('goog.a11y.aria.State');
goog.require('goog.array');
goog.require('goog.dom');
goog.require('goog.dom.TagName');
goog.require('goog.dom.classlist');
goog.require('goog.style');
goog.require('i18n.input.chrome.ElementType');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.MoreKeysShiftOperation');
goog.require('i18n.input.chrome.inputview.SpecNodeName');
goog.require('i18n.input.chrome.inputview.StateType');
goog.require('i18n.input.chrome.inputview.SwipeDirection');
goog.require('i18n.input.chrome.inputview.elements.content.FunctionalKey');
goog.require('i18n.input.chrome.inputview.util');
goog.require('i18n.input.chrome.message.ContextType');



goog.scope(function() {
var CompactKeyModel =
    i18n.input.chrome.inputview.elements.content.CompactKeyModel;
var ContextType = i18n.input.chrome.message.ContextType;
var MoreKeysShiftOperation = i18n.input.chrome.inputview.MoreKeysShiftOperation;
var SpecNodeName = i18n.input.chrome.inputview.SpecNodeName;
var StateType = i18n.input.chrome.inputview.StateType;
var util = i18n.input.chrome.inputview.util;



/**
 * The key to switch between different key set.
 *
 * @param {string} id The id.
 * @param {string} text The text.
 * @param {string} hintText The hint text.
 * @param {!i18n.input.chrome.inputview.StateManager} stateManager The state
 *     manager.
 * @param {boolean} hasShift True if the compact key has shift.}
 * @param {!CompactKeyModel} compactKeyModel The attributes of compact key.
 * @param {goog.events.EventTarget=} opt_eventTarget The event target.
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.content.FunctionalKey}
 */
i18n.input.chrome.inputview.elements.content.CompactKey = function(id, text,
    hintText, stateManager, hasShift, compactKeyModel, opt_eventTarget) {
  var textCssClass = compactKeyModel.textCssClass;
  goog.base(this, id, i18n.input.chrome.ElementType.
      COMPACT_KEY, text, '', opt_eventTarget, textCssClass);

  /**
   * The hint text.
   *
   * @type {string}
   */
  this.hintText = hintText;

  /** @private {boolean} */
  this.hasShift_ = hasShift;

  /**
   * The state manager.
   *
   * @type {!i18n.input.chrome.inputview.StateManager}
   * @private
   */
  this.stateManager_ = stateManager;

  /**
   * The attributes of compact key.
   *
   * @private {CompactKeyModel}
   */
  this.compactKeyModel_ = compactKeyModel;

  this.pointerConfig.longPressWithPointerUp = true;
  this.pointerConfig.flickerDirection =
      i18n.input.chrome.inputview.SwipeDirection.UP;
  this.pointerConfig.longPressDelay = 500;
};
goog.inherits(i18n.input.chrome.inputview.elements.content.CompactKey,
    i18n.input.chrome.inputview.elements.content.FunctionalKey);
var CompactKey = i18n.input.chrome.inputview.elements.content.CompactKey;


/**
 * The flickerred character.
 *
 * @type {string}
 */
CompactKey.prototype.flickerredCharacter = '';


/** @override */
CompactKey.prototype.createDom = function() {
  goog.base(this, 'createDom');

  goog.dom.classlist.add(this.tableCell,
      i18n.input.chrome.inputview.Css.COMPACT_KEY);
  if (!this.compactKeyModel_.isGrey) {
    goog.dom.classlist.remove(this.bgElem,
        i18n.input.chrome.inputview.Css.SPECIAL_KEY_BG);
  }

  if (this.hintText) {
    var dom = this.getDomHelper();
    dom.removeChildren(this.tableCell);
    this.inlineWrap = dom.createDom(goog.dom.TagName.DIV,
        i18n.input.chrome.inputview.Css.INLINE_DIV);
    dom.appendChild(this.tableCell, this.inlineWrap);
    this.hintTextElem = dom.createDom(goog.dom.TagName.DIV,
        i18n.input.chrome.inputview.Css.HINT_TEXT, this.hintText);
    dom.appendChild(this.inlineWrap, this.hintTextElem);
    dom.appendChild(this.inlineWrap, this.textElem);
  }

  goog.a11y.aria.setState(/** @type {!Element} */ (this.textElem),
      goog.a11y.aria.State.LABEL, this.text);
};


/** @override */
CompactKey.prototype.resize = function(width, height) {
  var elem = this.getElement();
  var marginLeft = Math.floor(width * this.compactKeyModel_.marginLeftPercent);
  if (marginLeft > 0) {
    marginLeft -= 5;
    elem.style.marginLeft = marginLeft + 'px';
  }
  var marginRight =
      Math.floor(width * this.compactKeyModel_.marginRightPercent);
  // TODO: Remove this ugly hack. The default margin right is 10px, we
  // need to add the default margin here to make all the keys have the same
  // look.
  if (marginRight > 0) {
    marginRight += 5;
    elem.style.marginRight = marginRight + 'px';
  }

  goog.base(this, 'resize', width, height);
};


/**
 * Gets the active character factoring in the current input type context.
 *
 * @return {string} The text to display on this key.
 * @private
 */
CompactKey.prototype.getContextOptimizedText_ = function() {
  var context = this.stateManager_.contextType;
  // Remove all old css rules.
  for (var contextType in this.compactKeyModel_.textOnContext) {
    var oldCss =
        this.compactKeyModel_.
        textOnContext[contextType][SpecNodeName.TEXT_CSS_CLASS];
    goog.dom.classlist.remove(this.tableCell, oldCss);
  }
  var text;
  if (context && this.compactKeyModel_.textOnContext[context]) {
    text = this.compactKeyModel_.textOnContext[context][SpecNodeName.TEXT];
    var newCss =
        this.compactKeyModel_.
        textOnContext[context][SpecNodeName.TEXT_CSS_CLASS];
    goog.dom.classlist.add(this.tableCell, newCss);
  } else if (this.hasShift_ && this.stateManager_.hasState(StateType.SHIFT)) {
    // When there is specific text to display when shift is pressed down,
    // the text should be set accordingly.
    text = this.compactKeyModel_.textOnShift ?
        this.compactKeyModel_.textOnShift : this.text.toUpperCase();
  } else {
    text = this.text;
  }
  return text;
};


/**
 * Get the active character. It may be upper case |text| when shift is pressed
 * or flickerred character when swipe. Note this should replace Compactkey.text
 * for compact keys.
 *
 * @return {string}
 */
CompactKey.prototype.getActiveCharacter = function() {
  if (this.flickerredCharacter) {
    return this.flickerredCharacter;
  } else {
    return this.getContextOptimizedText_();
  }
};


/** @override */
CompactKey.prototype.update = function() {
  goog.base(this, 'update');

  var text = this.getContextOptimizedText_();
  var displayHintText = this.stateManager_.contextType != ContextType.PASSWORD;
  if (this.compactKeyModel_.textOnShift) {
    if (this.hasShift_ && this.stateManager_.hasState(StateType.SHIFT)) {
      // Deal with the case that when shift is pressed down,
      // only one character should show in the compact key.
      displayHintText = false;
      text = this.compactKeyModel_.textOnShift;
    }
  }
  this.hintTextElem &&
      goog.style.setElementShown(this.hintTextElem, displayHintText);
  text = this.compactKeyModel_.title ?
      chrome.i18n.getMessage(this.compactKeyModel_.title) : text;
  goog.dom.setTextContent(this.textElem, text);


  var ariaLabel = text;
  if (this.hasShift_ && this.stateManager_.hasState(StateType.SHIFT) &&
      util.toUpper(text) != this.text) {
    // Punctuation keys do not change on shift, do not add cap to their aria
    // labels.
    ariaLabel = 'cap ' + text;
  }
  goog.a11y.aria.setState(/** @type {!Element} */ (this.textElem),
      goog.a11y.aria.State.LABEL, ariaLabel);
};


/**
 * Gets the more characters.
 *
 * @return {!Array.<string>} The characters.
 */
CompactKey.prototype.getMoreCharacters = function() {
  var context = this.stateManager_.contextType;
  var contextMap = context && this.compactKeyModel_.textOnContext[context];
  if (contextMap &&
      contextMap[SpecNodeName.MORE_KEYS] &&
      contextMap[SpecNodeName.MORE_KEYS][SpecNodeName.CHARACTERS]) {
    return goog.array.clone(
        contextMap[SpecNodeName.MORE_KEYS][SpecNodeName.CHARACTERS]);
  } else {
    var moreCharacters = goog.array.clone(this.compactKeyModel_.moreKeys);
    switch (this.compactKeyModel_.moreKeysShiftOperation) {
      case MoreKeysShiftOperation.TO_UPPER_CASE:
        if (this.getActiveCharacter().toLowerCase() !=
            this.getActiveCharacter()) {
          for (var i = 0; i < this.compactKeyModel_.moreKeys.length; i++) {
            moreCharacters[i] = this.compactKeyModel_.moreKeys[i].toUpperCase();
          }
          goog.array.removeDuplicates(moreCharacters);
        }
        return moreCharacters;
      case MoreKeysShiftOperation.TO_LOWER_CASE:
        if (this.hasShift_ && this.stateManager_.hasState(StateType.SHIFT)) {
          for (var i = 0; i < this.compactKeyModel_.moreKeys.length; i++) {
            moreCharacters[i] = this.compactKeyModel_.moreKeys[i].toLowerCase();
          }
          goog.array.removeDuplicates(moreCharacters);
        }
        return moreCharacters;
      case MoreKeysShiftOperation.STABLE:
        break;
    }
    return moreCharacters;
  }
};


/**
 * Gets the fixed number of columns to display accent characters for this key.
 *
 * @return {number} The fixed number of columns.
 */
CompactKey.prototype.getFixedColumns = function() {
  return this.compactKeyModel_.fixedColumns;
};

});  // goog.scope

