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
goog.provide('i18n.input.chrome.inputview.elements.content.GesturePreviewView');

goog.require('goog.dom.classlist');
goog.require('goog.object');
goog.require('goog.style');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.elements.Element');
goog.require('i18n.input.chrome.inputview.elements.ElementType');


goog.scope(function() {
var ElementType = i18n.input.chrome.inputview.elements.ElementType;


/**
 * Converts cooridnate in the keyboard window to coordinate in screen.
 *
 * @param {goog.math.Coordinate} coordinate The coordinate in keyboard window.
 * @return {goog.math.Coordinate} The cooridnate in screen.
 */
function convertToScreenCoordinate(coordinate) {
  var screenCoordinate = coordinate.clone();
  // This is a hack. Ideally, we should be able to use window.screenX/Y to get
  // cooridnate of the top left corner of VK window. But VK window is special
  // and the values are set to 0 all the time. We should fix the problem in
  // Chrome.
  screenCoordinate.y += screen.height - window.innerHeight;
  return screenCoordinate;
};



/**
 * The view for the gesture preview.
 *
 * @param {goog.events.EventTarget=} opt_eventTarget The parent event target.
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.Element}
 */
i18n.input.chrome.inputview.elements.content.GesturePreviewView = function(
    opt_eventTarget) {
  goog.base(this, '', ElementType.GESTURE_PREVIEW_VIEW, opt_eventTarget);

  /**
   * The window that shows the gesture preview.
   *
   * @type {chrome.app.window.AppWindow}
   * @private
   */
  this.gesturePreviewWindow_ = null;
};
goog.inherits(i18n.input.chrome.inputview.elements.content.GesturePreviewView,
    i18n.input.chrome.inputview.elements.Element);
var GesturePreviewView = i18n.input.chrome.inputview.elements.content.
    GesturePreviewView;


/**
 * The URL of the window which displays the gesture preview.
 *
 * @const {string}
 * @private
 */
GesturePreviewView.GESTURE_PREVIEW_URL_ = 'imewindows/gesturepreview.html';


/**
 * The ID of the floating window created by this view.
 *
 * @const {string}
 * @private
 */
GesturePreviewView.WINDOW_ID_ = 'gesture_preview_window';


/**
 * True if show was called and false if hide was called.
 *
 * @type {boolean}
 * @private
 */
GesturePreviewView.prototype.visible_ = false;


/** @override */
GesturePreviewView.prototype.createDom = function() {
  goog.base(this, 'createDom');

  // This class does not have any DOM in the input view.
};


/**
 * Shows gesture preview window.
 *
 * @param {!string} word The word to display in the preview window.
 */
GesturePreviewView.prototype.show = function(word) {
  if (!(chrome.app.window && chrome.app.window.create)) {
    // If the IME window is not available, do not show anything at all.
    return;
  }
  if (this.gesturePreviewWindow_ &&
      this.gesturePreviewWindow_.contentWindow.gesturePreview) {
    // If the window was already created, just update and show it.
    this.gesturePreviewWindow_.contentWindow.gesturePreview.setPreviewWord(
        word);
    this.gesturePreviewWindow_.show();
    this.visible_ = true;
    return;
  }
  this.visible_ = true;

  // TODO: Use convertToScreenCoordinate from altDataView to determine the
  // correct place to initially position the window.
  var windowBounds = goog.object.create('left', 0, 'top', 0, 'width', 140,
      'height', 40);
  var self = this;
  inputview.createWindow(
      chrome.runtime.getURL(GesturePreviewView.GESTURE_PREVIEW_URL_),
      goog.object.create('outerBounds', windowBounds, 'frame', 'none',
          'hidden', true, 'alphaEnabled', true,
          // Set the optional id parameter to ensure a unique window is created.
          'id', GesturePreviewView.WINDOW_ID_),
      function(newWindow) {
        self.gesturePreviewWindow_ = newWindow;
        var contentWindow = self.gesturePreviewWindow_.contentWindow;
        contentWindow.addEventListener('load', function() {
          contentWindow.gesturePreview.setPreviewWord(word);
          // Function hide maybe called before loading complete. Do not show
          // the window in this case.
          if (self.visible_) {
            self.gesturePreviewWindow_.show();
          } else {
            self.hide();
          }
        });
      });
};


/**
 * Hides the gesture preview window.
 */
GesturePreviewView.prototype.hide = function() {
  this.visible_ = false;
  if (this.gesturePreviewWindow_) {
    this.gesturePreviewWindow_.close();
    this.gesturePreviewWindow_ = null;
  }
  goog.style.setElementShown(this.getElement(), false);
};


/** @override */
GesturePreviewView.prototype.resize = function(width, height) {
  goog.base(this, 'resize', width, height);

  this.hide();
};

});  // goog.scope
