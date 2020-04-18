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
goog.provide('i18n.input.chrome.FloatingWindow');

goog.require('goog.dom.DomHelper');
goog.require('goog.dom.TagName');
goog.require('goog.math.Coordinate');
goog.require('goog.style');
goog.require('goog.ui.Container');
goog.require('i18n.input.chrome.Env');
goog.require('i18n.input.chrome.FloatingWindowDragger');


goog.scope(function() {
var Env = i18n.input.chrome.Env;
var FloatingWindowDragger = i18n.input.chrome.FloatingWindowDragger;



/**
 * The floating window in chrome OS.
 *
 * @param {!chrome.app.window.AppWindow} parentWindow The parent app window.
 * @param {boolean=} opt_enableDragger Whether to enable dragger feature.
 * @param {string=} opt_cssFile The optional css file.
 * @constructor
 * @extends {goog.ui.Container}
 */
i18n.input.chrome.FloatingWindow = function(parentWindow, opt_enableDragger,
    opt_cssFile) {
  goog.base(this, undefined, undefined,
      new goog.dom.DomHelper(parentWindow.contentWindow.document));

  /**
   * The parent app window.
   *
   * @protected {!chrome.app.window.AppWindow}
   */
  this.parentWindow = parentWindow;

  /** @protected {!Env} */
  this.env = Env.getInstance();

  /** @protected {boolean} */
  this.enableDragger = goog.isDef(opt_enableDragger) ? opt_enableDragger : true;

  if (opt_cssFile) {
    this.installCss(opt_cssFile);
  }
};
var FloatingWindow = i18n.input.chrome.FloatingWindow;
goog.inherits(FloatingWindow, goog.ui.Container);


/**
 * The dragger for floating window.
 *
 * @private {FloatingWindowDragger}
 */
FloatingWindow.prototype.dragger_;


/**
 * Install css in this windows.
 *
 * @param {string} file The css file.
 */
FloatingWindow.prototype.installCss = function(file) {
  var dh = this.getDomHelper();
  var doc = dh.getDocument();
  var head = dh.getElementsByTagNameAndClass('head')[0];

  var styleSheet = dh.createDom(goog.dom.TagName.LINK, {
    'rel': 'stylesheet',
    'href': file
  });
  dh.appendChild(head, styleSheet);
};


/** @override */
FloatingWindow.prototype.enterDocument = function() {
  goog.base(this, 'enterDocument');

  this.setFocusable(false);
  this.setFocusableChildrenAllowed(false);

  if (this.enableDragger) {
    this.dragger_ = new FloatingWindowDragger(
        this.parentWindow, this.getElement());
  }
};


/** @override */
FloatingWindow.prototype.setVisible = function(isVisible) {
  if (isVisible) {
    this.parentWindow.show();
  } else {
    this.parentWindow.hide();
  }
  return goog.base(this, 'setVisible', isVisible);
};


/**
 * Changes the position of the floating window.
 *
 * @param {!goog.math.Coordinate} position The coordinate of the position of the
 *     screen.
 */
FloatingWindow.prototype.reposition = function(position) {
  var outerBounds = this.parentWindow.outerBounds;
  if (outerBounds.left != position.x || outerBounds.top != position.y) {
    outerBounds.setPosition(position.x, position.y);
  }
};


/**
 * Gets the position of the floating window.
 *
 * @return {!goog.math.Coordinate} The top teft coordinate of the screen.
 */
FloatingWindow.prototype.getPosition = function() {
  var outerBounds = this.parentWindow.outerBounds;
  return new goog.math.Coordinate(outerBounds.left, outerBounds.top);
};


/**
 * Changes the size of the floating window.
 *
 * @param {number} width The width of the new window.
 * @param {number} height The height of the new window.
 */
FloatingWindow.prototype.resize = function(width, height) {
  this.parentWindow.outerBounds.setSize(width, height);
};


/**
 * Gets the size of the floating window.
 *
 * @return {!goog.math.Size} The element's size.
 */
FloatingWindow.prototype.size = function() {
  return goog.style.getSize(this.getElement());
};


/** @override */
FloatingWindow.prototype.disposeInternal = function() {
  this.parentWindow.close();
  goog.dispose(this.dragger_);
  goog.base(this, 'disposeInternal');
};
});  // goog.scope
