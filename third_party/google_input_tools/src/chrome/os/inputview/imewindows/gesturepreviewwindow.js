// Copyright 2016 The ChromeOS IME Authors. All Rights Reserved.
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
goog.provide('i18n.input.chrome.inputview.GesturePreviewWindow');

goog.require('goog.dom.TagName');
goog.require('goog.dom.classlist');
goog.require('goog.math.Coordinate');
goog.require('i18n.input.chrome.FloatingWindow');
goog.require('i18n.input.chrome.inputview.Css');


goog.scope(function() {
var Css = i18n.input.chrome.inputview.Css;
var FloatingWindow = i18n.input.chrome.FloatingWindow;
var TagName = goog.dom.TagName;



/**
 * The gesture preview window.
 *
 * @param {!chrome.app.window.AppWindow} parentWindow The parent app window.
 * @constructor
 * @extends {i18n.input.chrome.FloatingWindow}
 */
i18n.input.chrome.inputview.GesturePreviewWindow = function(parentWindow) {
  i18n.input.chrome.inputview.GesturePreviewWindow.base(
      this, 'constructor', parentWindow, undefined,
      GesturePreviewWindow.CSS_FILE_);
  this.setVisible(false);
  this.render();
};
var GesturePreviewWindow = i18n.input.chrome.inputview.GesturePreviewWindow;
goog.inherits(GesturePreviewWindow, FloatingWindow);


/**
 * The CSS file name for the gesture preview.
 *
 * @private @const {string}
 */
GesturePreviewWindow.CSS_FILE_ = 'gesturepreviewwindow_css.css';


/**
 * The total padding on the sides of the word.
 *
 * @private @const {number}
 */
GesturePreviewWindow.PADDING_SIDE_ = 70;


/**
 * The number of pixels to provide per character.
 *
 * @private @const {number}
 */
GesturePreviewWindow.PER_CHAR_WIDTH_ = 10;


/**
 * The height of the floating window.
 *
 * @private @const {number}
 */
GesturePreviewWindow.WINDOW_HEIGHT_ = 68;


/**
 * The vertical offset of the preview window.
 * This moves the window out from underneath the user's finger.
 *
 * @private @const {number}
 */
GesturePreviewWindow.Y_OFFSET_ = -140;


/**
 * The div that contains the preview text.
 *
 * @private {Element}
 */
GesturePreviewWindow.prototype.textDiv_;


/** @override */
GesturePreviewWindow.prototype.createDom = function() {
  GesturePreviewWindow.base(this, 'createDom');
  var container = this.getElement();
  goog.dom.classlist.add(container, Css.GESTURE_PREVIEW_CONTAINER);
};


/**
 * Shows the gesture preview window.
 *
 * @param {string} word The word to display in the preview window.
 */
GesturePreviewWindow.prototype.show = function(word) {
  this.setWord(word);
  this.resize(
      this.calculateWindowWidth_(word.length),
      GesturePreviewWindow.WINDOW_HEIGHT_);
  this.setVisible(true);
};


/**
 * Sets the word to show in the preview window.
 *
 * @param {string} word The word to show in the preview.
 */
GesturePreviewWindow.prototype.setWord = function(word) {
  if (!this.textDiv_) {
    var dom = this.getDomHelper();
    this.textDiv_ = dom.createDom(TagName.DIV, Css.GESTURE_PREVIEW_TEXT);
    dom.appendChild(this.getElement(), this.textDiv_);
  }
  this.textDiv_.textContent = word;
};


/**
 * Calculates the current width of the gesture preview window.
 *
 * @param {number} length The length of string to support.
 * @private
 * @return {number}
 */
GesturePreviewWindow.prototype.calculateWindowWidth_ = function(length) {
  // TODO(stevet): Use the true width of the word instead of this estimate.
  return GesturePreviewWindow.PADDING_SIDE_ +
      GesturePreviewWindow.PER_CHAR_WIDTH_ * length;
};


/**
 * Hides the gesture preview window.
 */
GesturePreviewWindow.prototype.hide = function() {
  this.setVisible(false);
};


/** @override */
GesturePreviewWindow.prototype.reposition = function(position) {
  position.y += Math.round(GesturePreviewWindow.Y_OFFSET_);
  position.x -= Math.round(this.parentWindow.contentWindow.innerWidth / 2);
  GesturePreviewWindow.base(
      this, 'reposition', convertToScreenCoordinate(position));
};


/**
 * Converts coordinate in the keyboard window to coordinate in screen.
 *
 * @param {!goog.math.Coordinate} coordinate The coordinate in keyboard window.
 * @return {!goog.math.Coordinate} The coordinate in screen.
 */
function convertToScreenCoordinate(coordinate) {
  var screenCoordinate = coordinate.clone();
  // This is a hack. Ideally, we should be able to use window.screenX/Y to get
  // coordinate of the top left corner of VK window. But VK window is special
  // and the values are set to 0 all the time. We should fix the problem in
  // Chrome.
  screenCoordinate.y += screen.height - window.innerHeight;
  return screenCoordinate;
};
});  // goog.scope
