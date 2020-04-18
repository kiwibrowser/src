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
goog.provide('i18n.input.chrome.inputview.elements.content.EnSwitcherKey');
goog.require('goog.dom.TagName');
goog.require('goog.dom.classlist');
goog.require('i18n.input.chrome.inputview.elements.content.FunctionalKey');


goog.scope(function() {



/**
 * The switcher key to switch to engish.
 *
 * @param {string} id The id.
 * @param {!i18n.input.chrome.ElementType} type The element
 *     type.
 * @param {string} text The text.
 * @param {i18n.input.chrome.inputview.Css} iconCssClass The css for the icon.
 * @param {!i18n.input.chrome.inputview.StateManager} stateManager The state
 *     manager.
 * @param {i18n.input.chrome.inputview.Css} defaultCss
 *     The Css for the default icon.
 * @param {i18n.input.chrome.inputview.Css} englishCss
 *     The Css for the english icon.
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.content.FunctionalKey}
 */
i18n.input.chrome.inputview.elements.content.EnSwitcherKey = function(id, type,
    text, iconCssClass, stateManager, defaultCss, englishCss) {
  i18n.input.chrome.inputview.elements.content.EnSwitcherKey.base(
      this, 'constructor', id, type, text, '');

  /**
   * The current icon css.
   *
   * @private {i18n.input.chrome.inputview.Css}
   */
  this.currentIconCss_ = defaultCss;

  /**
   * The state manager.
   *
   * @private {i18n.input.chrome.inputview.StateManager}
   */
  this.stateManager_ = stateManager;

  /**
   * The default iconCss for a given keyset.
   *
   * @private {i18n.input.chrome.inputview.Css}
   */
  this.defaultIconCss_ = defaultCss;

  /**
   * The iconCss for the english mode.
   *
   * @private {i18n.input.chrome.inputview.Css}
   */
  this.enIconCss_ = englishCss;
};
goog.inherits(i18n.input.chrome.inputview.elements.content.EnSwitcherKey,
    i18n.input.chrome.inputview.elements.content.FunctionalKey);
var EnSwitcherKey = i18n.input.chrome.inputview.elements.content.EnSwitcherKey;


/** @override */
EnSwitcherKey.prototype.createDom = function() {
  goog.base(this, 'createDom');
  var dom = this.getDomHelper();
  this.iconElem = dom.createDom(goog.dom.TagName.DIV, this.currentIconCss_);
  dom.appendChild(this.tableCell, this.iconElem);
};


/** @override */
EnSwitcherKey.prototype.update = function() {
  goog.base(this, 'update');
  var isEnMode = this.stateManager_.isEnMode;
  goog.dom.classlist.remove(this.iconElem, this.currentIconCss_);
  this.currentIconCss_ = isEnMode ? this.enIconCss_ : this.defaultIconCss_;
  goog.dom.classlist.add(this.iconElem, this.currentIconCss_);
};
});  // goog.scope
