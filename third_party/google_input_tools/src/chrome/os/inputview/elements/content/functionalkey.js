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
goog.provide('i18n.input.chrome.inputview.elements.content.FunctionalKey');

goog.require('goog.a11y.aria');
goog.require('goog.a11y.aria.State');
goog.require('goog.dom.TagName');
goog.require('goog.dom.classlist');
goog.require('i18n.input.chrome.ElementType');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.elements.content.SoftKey');


goog.scope(function() {
var ElementType = i18n.input.chrome.ElementType;



/**
 * The functional key, it could be modifier keys or keys like
 * backspace, tab, etc.
 *
 * @param {string} id The id.
 * @param {!i18n.input.chrome.ElementType} type The element
 *     type.
 * @param {string} text The text.
 * @param {string} iconCssClass The css class for the icon.
 * @param {goog.events.EventTarget=} opt_eventTarget The event target.
 * @param {string=} opt_textCssClass The css class for the text.
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.content.SoftKey}
 */
i18n.input.chrome.inputview.elements.content.FunctionalKey = function(id, type,
    text, iconCssClass, opt_eventTarget, opt_textCssClass) {
  goog.base(this, id, type, opt_eventTarget);

  /**
   * The text in the key.
   *
   * @type {string}
   */
  this.text = text;

  /**
   * The css class for the icon.
   *
   * @type {string}
   * @private
   */
  this.iconCssClass_ = iconCssClass;

  /**
   * The css class for the text.
   *
   * @type {string}
   * @private
   */
  this.textCssClass_ = opt_textCssClass || '';
};
goog.inherits(i18n.input.chrome.inputview.elements.content.FunctionalKey,
    i18n.input.chrome.inputview.elements.content.SoftKey);
var FunctionalKey = i18n.input.chrome.inputview.elements.content.FunctionalKey;


/**
 * The table cell.
 *
 * @type {!Element}
 */
FunctionalKey.prototype.tableCell;


/**
 * The element contains the text.
 *
 * @type {!Element}
 */
FunctionalKey.prototype.textElem;


/**
 * The element contains the icon.
 *
 * @type {!Element}
 */
FunctionalKey.prototype.iconElem;


/** @override */
FunctionalKey.prototype.createDom = function() {
  goog.base(this, 'createDom');

  var dom = this.getDomHelper();
  var elem = this.getElement();
  this.bgElem = dom.createDom(goog.dom.TagName.DIV,
      i18n.input.chrome.inputview.Css.SPECIAL_KEY_BG);
  dom.appendChild(elem, this.bgElem);
  this.tableCell = dom.createDom(goog.dom.TagName.DIV);
  goog.dom.classlist.add(this.tableCell,
      i18n.input.chrome.inputview.Css.MODIFIER);
  if (this.text) {
    this.textElem = dom.createDom(goog.dom.TagName.DIV,
        i18n.input.chrome.inputview.Css.SPECIAL_KEY_NAME, this.text);
    if (this.textCssClass_) {
      goog.dom.classlist.add(this.textElem, this.textCssClass_);
    }
    dom.appendChild(this.tableCell, this.textElem);
  }
  if (this.iconCssClass_) {
    this.iconElem = dom.createDom(goog.dom.TagName.DIV,
        this.iconCssClass_);
    dom.appendChild(this.tableCell, this.iconElem);
  }
  dom.appendChild(this.bgElem, this.tableCell);

  this.setAriaLabel(this.getChromeVoxMessage());
};


/** @override */
FunctionalKey.prototype.resize = function(width, height) {
  goog.base(this, 'resize', width, height);

  this.tableCell.style.width = this.availableWidth + 'px';
  this.tableCell.style.height = this.availableHeight + 'px';
};


/** @override */
FunctionalKey.prototype.setHighlighted = function(highlight) {
  if (highlight) {
    goog.dom.classlist.add(this.bgElem,
        i18n.input.chrome.inputview.Css.SPECIAL_KEY_HIGHLIGHT);
  } else {
    goog.dom.classlist.remove(this.bgElem,
        i18n.input.chrome.inputview.Css.SPECIAL_KEY_HIGHLIGHT);
  }
};


/**
 * Some keys don't need background highlight, use this method to
 * disable it.
 */
FunctionalKey.prototype.disableBackgroundHighlight = function() {
  goog.dom.classlist.add(this.bgElem, i18n.input.chrome.inputview.Css.
      SPECIAL_KEY_DISABLE_BG_HIGHLIGHT);
};


/**
 * Gets the chrome vox message.
 *
 * @return {string} .
 */
FunctionalKey.prototype.getChromeVoxMessage = function() {
  switch (this.type) {
    case ElementType.BACKSPACE_KEY:
      return chrome.i18n.getMessage('BACKSPACE');
    case ElementType.ENTER_KEY:
      return chrome.i18n.getMessage('ENTER');
    case ElementType.TAB_KEY:
      return chrome.i18n.getMessage('TAB');
    case ElementType.ARROW_UP:
      return chrome.i18n.getMessage('UP_ARROW');
    case ElementType.ARROW_DOWN:
      return chrome.i18n.getMessage('DOWN_ARROW');
    case ElementType.ARROW_LEFT:
      return chrome.i18n.getMessage('LEFT_ARROW');
    case ElementType.ARROW_RIGHT:
      return chrome.i18n.getMessage('RIGHT_ARROW');
    case ElementType.HIDE_KEYBOARD_KEY:
      return chrome.i18n.getMessage('HIDE_KEYBOARD');
    case ElementType.GLOBE_KEY:
      return chrome.i18n.getMessage('GLOBE');
    case ElementType.MENU_KEY:
      return chrome.i18n.getMessage('MENU_KEY');
  }
  return '';
};


/**
 * Sets the aria label.
 *
 * @param {string} label .
 */
FunctionalKey.prototype.setAriaLabel = function(label) {
  var elem = this.textElem || this.iconElem;
  if (elem) {
    goog.a11y.aria.setState(elem, goog.a11y.aria.State.LABEL, label);
  }
};

});  // goog.scope

