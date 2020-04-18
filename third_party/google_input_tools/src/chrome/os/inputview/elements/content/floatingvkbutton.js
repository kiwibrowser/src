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
goog.provide('i18n.input.chrome.inputview.elements.content.FloatingVKButton');

goog.require('goog.a11y.aria.State');
goog.require('goog.dom.TagName');
goog.require('goog.dom.classlist');
goog.require('goog.fx.Dragger');
goog.require('i18n.input.chrome.FloatingWindowDragger');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.elements.Element');

goog.scope(function() {
var Css = i18n.input.chrome.inputview.Css;



/**
 * The resize button.
 *
 * @param {string} id The id.
 * @param {!i18n.input.chrome.inputview.elements.ElementType} type The element
 *     type.
 * @param {string} iconCssClass The css class for the icon.
 * @param {goog.events.EventTarget=} opt_eventTarget The event target.
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.Element}
 */
i18n.input.chrome.inputview.elements.content.FloatingVKButton = function(id,
    type, iconCssClass, opt_eventTarget) {
  goog.base(this, id, type, opt_eventTarget);

  /**
   * The css class for the icon.
   *
   * @private {string}
   */
  this.iconCssClass_ = iconCssClass;
};
goog.inherits(i18n.input.chrome.inputview.elements.content.FloatingVKButton,
    i18n.input.chrome.inputview.elements.Element);
var FloatingVKButton =
    i18n.input.chrome.inputview.elements.content.FloatingVKButton;


/** @private {!Element} */
FloatingVKButton.prototype.iconCell_;


/** @override */
FloatingVKButton.prototype.createDom = function() {
  goog.base(this, 'createDom');
  var elem = this.getElement();
  var dom = goog.dom.getDomHelper();
  goog.dom.classlist.addAll(elem, [Css.CANDIDATE_INTER_CONTAINER,
      Css.TOOLBAR_BUTTON, Css.FLOAT_LEFT]);
  this.iconCell_ = dom.createDom(goog.dom.TagName.DIV, Css.TABLE_CELL);
  dom.appendChild(elem, this.iconCell_);

  var iconElem = dom.createDom(goog.dom.TagName.DIV, Css.INLINE_DIV);
  goog.dom.classlist.add(iconElem, this.iconCssClass_);
  dom.appendChild(this.iconCell_, iconElem);

  //TODO: setup aria label.
};


/** @override */
FloatingVKButton.prototype.resize = function(width, height) {
  goog.style.setSize(this.iconCell_, width, height);
  goog.base(this, 'resize', width, height);
};


});

