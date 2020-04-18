// Copyright 2015 The ChromeOS IME Authors. All Rights Reserved.
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
goog.provide('i18n.input.chrome.inputview.elements.content.DragButton');

goog.require('goog.a11y.aria');
goog.require('goog.a11y.aria.State');
goog.require('goog.dom');
goog.require('goog.dom.TagName');
goog.require('goog.dom.classlist');
goog.require('goog.style');
goog.require('i18n.input.chrome.FloatingWindowDragger');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.elements.Element');


goog.scope(function() {
var Css = i18n.input.chrome.inputview.Css;



/**
 * The drag button.
 *
 * @param {string} id The id.
 * @param {!i18n.input.chrome.ElementType} type The element
 *     type.
 * @param {string} iconCssClass The css class for the icon.
 * @param {goog.events.EventTarget=} opt_eventTarget The event target.
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.Element}
 */
i18n.input.chrome.inputview.elements.content.DragButton = function(id, type,
    iconCssClass, opt_eventTarget) {
  i18n.input.chrome.inputview.elements.content.DragButton.base(this,
      'constructor', id, type, opt_eventTarget);

  /**
   * The css class for the icon.
   *
   * @private {string}
   */
  this.iconCssClass_ = iconCssClass;

  this.pointerConfig.stopEventPropagation = false;
};
goog.inherits(i18n.input.chrome.inputview.elements.content.DragButton,
    i18n.input.chrome.inputview.elements.Element);
var DragButton = i18n.input.chrome.inputview.elements.content.DragButton;


/** @private {!i18n.input.chrome.FloatingWindowDragger} */
DragButton.prototype.dragger_;


/** @private {!Element} */
DragButton.prototype.iconCell_;


/** @override */
DragButton.prototype.createDom = function() {
  DragButton.base(this, 'createDom');
  var elem = this.getElement();
  var dom = goog.dom.getDomHelper();
  goog.dom.classlist.addAll(elem, [Css.CANDIDATE_INTER_CONTAINER,
    Css.TOOLBAR_BUTTON, Css.FLOAT_LEFT]);
  this.iconCell_ = dom.createDom(goog.dom.TagName.DIV, Css.TABLE_CELL);
  dom.appendChild(elem, this.iconCell_);

  var iconElem = dom.createDom(goog.dom.TagName.DIV, Css.INLINE_DIV);
  goog.dom.classlist.add(iconElem, this.iconCssClass_);
  dom.appendChild(this.iconCell_, iconElem);

  var ariaLabel = chrome.i18n.getMessage('DRAG_BUTTON');
  goog.a11y.aria.setState(/** @type {!Element} */ (elem),
      goog.a11y.aria.State.LABEL, ariaLabel);
};


/** @override */
DragButton.prototype.enterDocument = function() {
  DragButton.base(this, 'enterDocument');
  this.dragger_ = new i18n.input.chrome.FloatingWindowDragger(window,
      this.getElement());
};


/** @override */
DragButton.prototype.resize = function(width, height) {
  goog.style.setSize(this.iconCell_, width, height);
  DragButton.base(this, 'resize', width, height);
};


});  // goog.scope
