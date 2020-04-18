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
goog.provide('i18n.input.chrome.inputview.elements.Element');

goog.require('goog.dom.classlist');
goog.require('goog.events.EventHandler');
goog.require('goog.style');
goog.require('goog.ui.Component');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.PointerConfig');


goog.scope(function() {



/**
 * The abstract class for element in input view keyboard.
 *
 * @param {string} id The id.
 * @param {!i18n.input.chrome.ElementType} type The element
 *     type.
 * @param {goog.events.EventTarget=} opt_eventTarget The event target.
 * @constructor
 * @extends {goog.ui.Component}
 */
i18n.input.chrome.inputview.elements.Element = function(id, type,
    opt_eventTarget) {
  goog.base(this);
  this.setParentEventTarget(opt_eventTarget || null);

  /**
   * The id of the element.
   *
   * @type {string}
   */
  this.id = id;

  /**
   * The type of the element.
   *
   * @type {!i18n.input.chrome.ElementType}
   */
  this.type = type;

  /**
   * The display of the element.
   *
   * @type {string}
   * @private
   */
  this.display_ = '';

  /**
   * The event handler.
   *
   * @type {!goog.events.EventHandler}
   */
  this.handler = new goog.events.EventHandler(this);

  /**
   * The configuration for the pointer.
   *
   * @type {!i18n.input.chrome.inputview.PointerConfig}
   */
  this.pointerConfig = new i18n.input.chrome.inputview.PointerConfig(false,
      false, false);
};
goog.inherits(i18n.input.chrome.inputview.elements.Element, goog.ui.Component);
var Element = i18n.input.chrome.inputview.elements.Element;


/**
 * The width of the element.
 *
 * @type {number}
 */
Element.prototype.width;


/**
 * The height of the element.
 *
 * @type {number}
 */
Element.prototype.height;


/**
 * Resizes the element.
 *
 * @param {number} width The total width.
 * @param {number} height The total height.
 */
Element.prototype.resize = function(width, height) {
  this.width = width;
  this.height = height;
};


/** @override */
Element.prototype.createDom = function() {
  goog.base(this, 'createDom');

  this.getElement().id = this.id;
  this.getElement()['view'] = this;
};


/** @override */
Element.prototype.enterDocument = function() {
  goog.base(this, 'enterDocument');

  this.display_ = this.getElement().style.display;
};


/**
 * Whether the element is visible.
 *
 * @return {boolean} True if the element is visible.
 */
Element.prototype.isVisible = function() {
  return goog.style.isElementShown(this.getElement());
};


/**
 * Sets the visibility of the element.
 *
 * @param {boolean} visibility True if the element is visible.
 */
Element.prototype.setVisible = function(visibility) {
  // TODO: Figure out why element can be null.
  var element = this.getElement();
  if (element)
    element.style.display = visibility ? this.display_ : 'none';
};


/**
 * Updates the element.
 */
Element.prototype.update = function() {
  this.setHighlighted(false);
  for (var i = 0; i < this.getChildCount(); i++) {
    var child = /** @type {!Element} */ (
        this.getChildAt(i));
    child.update();
  }
};


/**
 * Sets the highlight of the soft key.
 *
 * @param {boolean} highlight True to set it to be highlighted.
 */
Element.prototype.setHighlighted = function(
    highlight) {
  if (highlight) {
    goog.dom.classlist.add(this.getElement(),
        i18n.input.chrome.inputview.Css.ELEMENT_HIGHLIGHT);
  } else {
    goog.dom.classlist.remove(this.getElement(),
        i18n.input.chrome.inputview.Css.ELEMENT_HIGHLIGHT);
  }
};


/** @override */
Element.prototype.disposeInternal = function() {
  this.getElement()['view'] = null;
  goog.dispose(this.handler);

  goog.base(this, 'disposeInternal');
};

});  // goog.scope
