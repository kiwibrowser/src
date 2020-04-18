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
goog.provide('i18n.input.chrome.inputview.elements.content.TabBarKey');

goog.require('goog.a11y.aria');
goog.require('goog.a11y.aria.State');
goog.require('goog.dom.TagName');
goog.require('goog.dom.classlist');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.EmojiType');
goog.require('i18n.input.chrome.inputview.elements.content.FunctionalKey');

goog.scope(function() {

var Css = i18n.input.chrome.inputview.Css;
var EmojiType = i18n.input.chrome.inputview.EmojiType;



/**
 * The Tabbar key
 *
 * @param {string} id The id.
 * @param {!i18n.input.chrome.ElementType} type The element
 *     type.
 * @param {string} text The text.
 * @param {string} iconCssClass The css class for the icon.
 * @param {number} toCategory The category the tabbar key represents.
 * @param {i18n.input.chrome.inputview.StateManager} stateManager
 *     The state manager.
 * @param {goog.events.EventTarget=} opt_eventTarget The event target.
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.content.FunctionalKey}
 */
i18n.input.chrome.inputview.elements.content.TabBarKey = function(id, type,
    text, iconCssClass, toCategory, stateManager, opt_eventTarget) {
  goog.base(this, id, type, text, iconCssClass, opt_eventTarget);

  /**
   * The id of the key set to go after this switcher key in menu is pressed.
   *
   * @type {number}
   */
  this.toCategory = toCategory;

  /**
   * The state manager.
   *
   * @type {i18n.input.chrome.inputview.StateManager}
   */
  this.stateManager = stateManager;

  this.pointerConfig.stopEventPropagation = false;
};
goog.inherits(i18n.input.chrome.inputview.elements.content.TabBarKey,
    i18n.input.chrome.inputview.elements.content.FunctionalKey);
var TabBarKey = i18n.input.chrome.inputview.elements.content.TabBarKey;


/**
 * The height of the bottom border
 *
 * @type {number}
 * @private
 */
TabBarKey.prototype.BORDER_HEIGHT_ = 4;


/** @override */
TabBarKey.prototype.createDom = function() {
  goog.base(this, 'createDom');
  var elem = this.getElement();
  goog.dom.classlist.remove(elem, Css.SOFT_KEY);
  goog.dom.classlist.add(elem, Css.EMOJI_TABBAR_SK);
  goog.dom.classlist.remove(this.bgElem, Css.SPECIAL_KEY_BG);
  goog.dom.classlist.add(this.bgElem, Css.EMOJI_TABBAR_KEY);
  goog.dom.classlist.add(this.iconElem, Css.EMOJI_SWITCH);

  // Sets aria label.
  var ariaLabel = '';
  switch (this.toCategory) {
    case EmojiType.RECENT:
      ariaLabel = chrome.i18n.getMessage('EMOJI_TAB_RECENT');
      break;
    case EmojiType.HOT:
      ariaLabel = chrome.i18n.getMessage('EMOJI_TAB_HOT');
      break;
    case EmojiType.EMOTION:
      ariaLabel = chrome.i18n.getMessage('EMOJI_TAB_FACE');
      break;
    case EmojiType.SPECIAL_CHARACTERS:
      ariaLabel = chrome.i18n.getMessage('EMOJI_TAB_SYMBOL');
      break;
    case EmojiType.NATURE:
      ariaLabel = chrome.i18n.getMessage('EMOJI_TAB_NATURE');
      break;
    case EmojiType.PLACES_OF_INTERESTS:
      ariaLabel = chrome.i18n.getMessage('EMOJI_TAB_PLACE');
      break;
    case EmojiType.ITEMS:
      ariaLabel = chrome.i18n.getMessage('EMOJI_TAB_OBJECT');
      break;
    case EmojiType.EMOTICON:
      ariaLabel = chrome.i18n.getMessage('EMOJI_TAB_EMOTICON');
      break;
  }
  goog.a11y.aria.setState(/** @type {!Element} */ (elem),
      goog.a11y.aria.State.LABEL, ariaLabel);
};


/** @override */
TabBarKey.prototype.resize = function(width,
    height) {
  goog.base(this, 'resize', width, height);
  this.tableCell.style.width = this.availableWidth + 'px';
  this.tableCell.style.height = this.availableHeight -
      this.BORDER_HEIGHT_ + 'px';
};


/**
 * Create the separator for the tabbar key.
 *
 * @private
 */
TabBarKey.prototype.createSeparator_ = function() {
  var dom = this.getDomHelper();
  this.sepTableCell = dom.createDom(goog.dom.TagName.DIV, Css.TABLE_CELL);
  this.separator = dom.createDom(goog.dom.TagName.DIV,
      Css.CANDIDATE_SEPARATOR);
  this.separator.style.height = Math.floor(this.height * 0.32) + 'px';
  dom.appendChild(this.sepTableCell, this.separator);
  dom.appendChild(this.bgElem, this.sepTableCell);
};


/**
 * Update the border.
 *
 * @param {number} categoryID the categoryID.
 */
TabBarKey.prototype.updateBorder = function(categoryID) {
  if (categoryID == this.toCategory) {
    goog.dom.classlist.add(this.bgElem, Css.EMOJI_TABBAR_KEY_HIGHLIGHT);
    goog.dom.classlist.add(this.iconElem, Css.EMOJI_SWITCH_HIGHLIGHT);
  } else {
    goog.dom.classlist.remove(this.bgElem, Css.EMOJI_TABBAR_KEY_HIGHLIGHT);
    goog.dom.classlist.remove(this.iconElem, Css.EMOJI_SWITCH_HIGHLIGHT);
  }
};
});  // goog.scope
