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
goog.provide('i18n.input.chrome.inputview.elements.content.FloatingView');

goog.require('goog.dom.classlist');
goog.require('goog.style');
goog.require('i18n.input.chrome.ElementType');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.elements.Element');


goog.scope(function() {
var ElementType = i18n.input.chrome.ElementType;



/**
 * The view for floating virtual keyboard. It is used when the user drags the
 * virtual keyboard around or resizes.
 *
 * @param {goog.events.EventTarget=} opt_eventTarget The parent event target.
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.Element}
 */
i18n.input.chrome.inputview.elements.content.FloatingView = function(
    opt_eventTarget) {
  goog.base(this, '', ElementType.FLOATING_VIEW, opt_eventTarget);

  this.pointerConfig.stopEventPropagation = false;
};
goog.inherits(i18n.input.chrome.inputview.elements.content.FloatingView,
    i18n.input.chrome.inputview.elements.Element);
var FloatingView = i18n.input.chrome.inputview.elements.content.FloatingView;


/** @override */
FloatingView.prototype.createDom = function() {
  goog.base(this, 'createDom');
  var elem = this.getElement();
  goog.dom.classlist.add(elem, i18n.input.chrome.inputview.Css.FLOATING_COVER);
};


/** @override */
FloatingView.prototype.enterDocument = function() {
  goog.base(this, 'enterDocument');
  goog.style.setElementShown(this.getElement(), false);
};


/** @override */
FloatingView.prototype.resize = function(width, height) {
  goog.base(this, 'resize', width, height);
  goog.style.setSize(this.getElement(), width, height);
};


/**
 * Shows the floating view.
 */
FloatingView.prototype.show = function() {
  goog.style.setElementShown(this.getElement(), true);
};


/**
 * Hides the floating view.
 */
FloatingView.prototype.hide = function() {
  goog.style.setElementShown(this.getElement(), false);
};

});  // goog.scope
