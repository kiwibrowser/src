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
goog.provide('i18n.input.chrome.inputview.elements.content.SpaceKey');

goog.require('goog.dom');
goog.require('goog.dom.TagName');
goog.require('goog.dom.classlist');
goog.require('goog.style');
goog.require('i18n.input.chrome.ElementType');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.StateType');
goog.require('i18n.input.chrome.inputview.elements.content.SoftKey');



goog.scope(function() {
var Css = i18n.input.chrome.inputview.Css;
var content = i18n.input.chrome.inputview.elements.content;



/**
 * The space key.
 *
 * @param {string} id the id.
 * @param {!i18n.input.chrome.inputview.StateManager} stateManager The state
 *     manager.
 * @param {string} title The keyboard title.
 * @param {!Array.<string>=} opt_characters The characters.
 * @param {goog.events.EventTarget=} opt_eventTarget The event target.
 * @param {string=} opt_iconCss The icon CSS class
 * @param {string=} opt_toKeyset The id of the keyset to switch to if spacebar
 *     is pressed after a symbol.
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.content.SoftKey}
 */
content.SpaceKey = function(id, stateManager, title, opt_characters,
    opt_eventTarget, opt_iconCss, opt_toKeyset) {
  goog.base(this, id, i18n.input.chrome.ElementType.
      SPACE_KEY, opt_eventTarget);

  /**
   * The characters.
   *
   * @type {!Array.<string>}
   * @private
   */
  this.characters_ = opt_characters || [];

  /**
   * The title on the space key.
   *
   * @private {string}
   */
  this.title_ = title;

  /**
   * The state manager.
   *
   * @type {!i18n.input.chrome.inputview.StateManager}
   * @private
   */
  this.stateManager_ = stateManager;

  /** @private {string} */
  this.iconCss_ = opt_iconCss || '';

  /**
   * The id of the keyset to go to (if necessary).
   * This is used to return to the letter keyset when pressing space
   * after a symbol.
   *
   * @type {string}
   */
  this.toKeyset = opt_toKeyset || '';

  // Double click on space key may convert two spaces to a period followed by a
  // space.
  this.pointerConfig.dblClick = true;
  this.pointerConfig.dblClickDelay = 1000;
};
goog.inherits(content.SpaceKey, i18n.input.chrome.inputview.elements.
    content.SoftKey);
var SpaceKey = content.SpaceKey;


/**
 * The height of the space key.
 *
 * @type {number}
 */
SpaceKey.HEIGHT = 43;


/**
 * The wrapper inside the space key.
 *
 * @private {!Element}
 */
SpaceKey.prototype.wrapper_;


/** @override */
SpaceKey.prototype.createDom = function() {
  goog.base(this, 'createDom');

  var dom = this.getDomHelper();
  this.wrapper_ = dom.createDom(goog.dom.TagName.DIV, Css.SPACE_WRAPPER);
  dom.appendChild(this.getElement(), this.wrapper_);
  if (this.iconCss_) {
    var iconElem = dom.createDom(goog.dom.TagName.DIV, this.iconCss_);
    dom.appendChild(this.wrapper_, iconElem);
  } else {
    goog.dom.classlist.add(this.wrapper_, Css.SPACE_GREY_BG);
    goog.dom.setTextContent(this.wrapper_, this.title_);
  }
};


/**
 * Gets the character.
 *
 * @return {string} The character.
 */
SpaceKey.prototype.getCharacter = function() {
  if (this.characters_) {
    // The index is based on the characters in order:
    // 0: Default
    // 1: Shift
    // 2: ALTGR
    // 3: SHIFT + ALTGR
    var index = this.stateManager_.hasState(i18n.input.chrome.inputview.
        StateType.SHIFT) ? 1 : 0 + this.stateManager_.hasState(
            i18n.input.chrome.inputview.StateType.ALTGR) ? 2 : 0;
    if (this.characters_.length > index && this.characters_[index]) {
      return this.characters_[index];
    }
  }
  return ' ';
};


/**
 * Updates the title on the space key.
 *
 * @param {string} title .
 * @param {boolean} visible True to set title visible.
 */
SpaceKey.prototype.updateTitle = function(title, visible) {
  if (this.iconCss_) {
    return;
  }
  this.text = title;
  goog.dom.setTextContent(this.wrapper_, visible ? title : '');
  goog.dom.classlist.add(this.wrapper_,
      i18n.input.chrome.inputview.Css.TITLE);
};


/** @override */
SpaceKey.prototype.setHighlighted = function(highlight) {
  if (highlight) {
    goog.dom.classlist.add(this.wrapper_, i18n.input.chrome.inputview.Css.
        ELEMENT_HIGHLIGHT);
  } else {
    goog.dom.classlist.remove(this.wrapper_, i18n.input.chrome.inputview.Css.
        ELEMENT_HIGHLIGHT);
  }
};


/**
 * Gets the message for chromevox.
 *
 * @return {string} .
 */
SpaceKey.prototype.getChromeVoxMessage = function() {
  return chrome.i18n.getMessage('SPACE');
};


/** @override */
SpaceKey.prototype.resize = function(width, height) {
  goog.base(this, 'resize', width, height);

  goog.style.setSize(this.wrapper_, width, SpaceKey.HEIGHT);
  // Positions the wrapper in the middle.
  this.wrapper_.style.top = (this.availableHeight - SpaceKey.HEIGHT) / 2 + 'px';
};

});  // goog.scope
