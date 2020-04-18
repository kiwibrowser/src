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
/**
 * @fileoverview The canvas that handles gesture typing for inputview.
 */
goog.provide('i18n.input.chrome.inputview.elements.content.GestureCanvasView');

goog.require('goog.dom.TagName');
goog.require('goog.dom.classlist');
goog.require('goog.style');
goog.require('i18n.input.chrome.ElementType');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.elements.Element');
goog.require('i18n.input.chrome.inputview.elements.content.GestureStroke');
goog.require('i18n.input.chrome.inputview.elements.content.Point');

goog.scope(function() {
var Css = i18n.input.chrome.inputview.Css;
var ElementType = i18n.input.chrome.ElementType;
var GestureStroke = i18n.input.chrome.inputview.elements.content.GestureStroke;
var Point = i18n.input.chrome.inputview.elements.content.Point;
var TagName = goog.dom.TagName;



/**
 * The gesture canvas view.
 *
 * This view is used to display the strokes for gesture typing, and handles
 * stroke lifetime management and rendering logic.
 *
 * @param {goog.events.EventTarget=} opt_eventTarget The parent event target.
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.Element}
 */
i18n.input.chrome.inputview.elements.content.GestureCanvasView =
    function(opt_eventTarget) {
  GestureCanvasView.base(this, 'constructor', '',
      ElementType.GESTURE_CANVAS_VIEW, opt_eventTarget);

  /**
   * Flag used to indicate whether or not gesturing is currently occuring.
   *
   * @type {boolean}
   */
  this.isGesturing = false;

  /**
   * Actual canvas for drawing the gesture trail.
   *
   * @private {!Element}
   */
  this.drawingCanvas_;

  /**
   * Context for drawing the gesture trail.
   *
   * @private {!CanvasRenderingContext2D}
   */
  this.drawingContext_;

  /**
   * A list of list of gesture points to be rendered on the canvas as strokes.
   *
   * @private {!Array<!GestureStroke>}
   */
  this.strokeList_ = [];
};
var GestureCanvasView =
    i18n.input.chrome.inputview.elements.content.GestureCanvasView;
goog.inherits(GestureCanvasView, i18n.input.chrome.inputview.elements.Element);


/**
 * Draws the gesture trail.
 *
 * @private
 */
GestureCanvasView.prototype.draw_ = function() {
  // First, clear the canvas.
  this.drawingContext_.clearRect(
      0, 0, this.drawingCanvas_.width, this.drawingCanvas_.height);

  for (var i = 0; i < this.strokeList_.length; i++) {
    this.strokeList_[i].draw(this.drawingContext_);
  }
};


/** @override */
GestureCanvasView.prototype.createDom = function() {
  GestureCanvasView.base(this, 'createDom');

  var dom = this.getDomHelper();
  var elem = this.getElement();
  goog.dom.classlist.add(elem, Css.GESTURE_CANVAS_VIEW);

  // Create the HTML5 canvas where the gesture trail is actually rendered.
  this.drawingCanvas_ = dom.createDom(TagName.CANVAS, Css.DRAWING_CANVAS);
  this.drawingContext_ = this.drawingCanvas_.getContext('2d');
  dom.appendChild(elem, this.drawingCanvas_);

  window.requestAnimationFrame(this.animateGestureTrail_.bind(this));
};


/** @override */
GestureCanvasView.prototype.resize = function(width, height) {
  GestureCanvasView.base(this, 'resize', width, height);

  // Explicitly set the width and height of the canvas, which is necessary
  // to ensure that rendered elements are not stretched.
  this.drawingCanvas_.width = width;
  this.drawingCanvas_.height = height;
};


/**
 * Converts a drag event to a gesture Point and adds it to the collection of
 * points.
 *
 * @param {!i18n.input.chrome.inputview.events.DragEvent} e Drag event to draw.
 */
GestureCanvasView.prototype.addPoint = function(e) {
  // Check if the last stroke was active before this point in order to determine
  // if the user is gesturing. Only check the last stroke and not all the
  // strokes because all previous strokes might be rendering/degrading, but that
  // does not determine if the user is currently gesturing.
  var wasActive = this.latestStrokeActive_();

  if (this.strokeList_.length == 0) {
    this.strokeList_.push(new GestureStroke());
  }
  var lastStroke = this.strokeList_[this.strokeList_.length - 1];
  if (lastStroke.length() > 0 && !this.isActiveIdentifier(e.identifier)) {
    // Should only add new points with the same identifier. This ignores pointer
    // events created by, say, a second finger interacting with the screen while
    // an existing gesture is going on.
    return;
  }
  lastStroke.pushPoint(this.createGesturePoint_(e));

  // If the new point |e| activated the last stroke, set gesturing to true.
  if (!wasActive && this.latestStrokeActive_()) {
    this.isGesturing = true;
  }
};


/**
 * Clears the view.
 */
GestureCanvasView.prototype.clear = function() {
  this.strokeList_ = [];
  this.draw_();
};


/**
 * @return {boolean} Whether or not the last stroke is active.
 * @private
 */
GestureCanvasView.prototype.latestStrokeActive_ = function() {
  if (this.strokeList_.length == 0) {
    return false;
  }
  return this.strokeList_[this.strokeList_.length - 1].isActive();
};


/**
 * Begins a new gesture.
 *
 * @param {!i18n.input.chrome.inputview.events.PointerEvent} e Drag event to
 *     draw.
 */
GestureCanvasView.prototype.startStroke = function(e) {
  // If there is currently a stroke and it does not match the identifier of this
  // new point, then ignore this call. This is to prevent a second finger from
  // interrupting an existing stroke.
  if (this.strokeList_.length > 0 && !this.isActiveIdentifier(e.identifier)) {
    return;
  }
  // Always start a new array to separate previous strokes from this new one.
  this.strokeList_.push(new GestureStroke());
  var point = this.createGesturePoint_(e);
  point.action = Point.Action.DOWN;
  // TODO(stevet): This line is a NOP since createGesturePoint_ already assigns
  // the pointer value, but it must be called to prevent closure from optimizing
  // out the pointer member. This needs to be fixed to use the true pointer ID
  // of e.
  point.pointer = 0;
  this.strokeList_[this.strokeList_.length - 1].pushPoint(point);
};


/**
 * Ends the current gesture.
 *
 * @param {!i18n.input.chrome.inputview.events.PointerEvent} e Final pointer
 *     event to handle.
 */
GestureCanvasView.prototype.endStroke = function(e) {
  // TODO(stevet): Ensure that this gets called even when the final touch event
  // is not on the client.

  // Ignore points that do not have the same identifier.
  if (e.identifier !=
      this.strokeList_[this.strokeList_.length - 1].getIdentifierAt(0)) {
    return;
  }

  // Send the final event.
  var point = this.createGesturePoint_(e);
  point.action = Point.Action.UP;
  this.strokeList_[this.strokeList_.length - 1].pushPoint(point);
  this.isGesturing = false;
};


/**
 * Returns the last stroke, or null if there are currently no strokes.
 *
 * @return {?GestureStroke}
 */
GestureCanvasView.prototype.getLastStroke = function() {
  if (this.strokeList_.length == 0) {
    return null;
  }
  return this.strokeList_[this.strokeList_.length - 1];
};


/**
 * @param {number} identifier The identifier to check.
 * @return {boolean} Whether or not identifier is the same as the identifier of
 *     the current active stroke.
 */
GestureCanvasView.prototype.isActiveIdentifier = function(identifier) {
  return identifier == this.strokeList_[this.strokeList_.length - 1]
      .getIdentifierAt(0);
};


/**
 * Removes only empty strokes from the stroke list.
 */
GestureCanvasView.prototype.removeEmptyStrokes = function() {
  for (var i = 0; i < this.strokeList_.length; i++) {
    if (this.strokeList_[i].isDegraded()) {
      this.strokeList_.splice(i, 1);
      i--;
    }
  }
};


/**
 * Animates the gesture trail.
 *
 * @private
 */
GestureCanvasView.prototype.animateGestureTrail_ = function() {
  // TODO(stevet): Currently these two methods assume callback at 60fps. They
  // should technically be modified to re-draw and update based on the actual
  // time passed.
  this.draw_();
  this.degradeStrokes_();
  window.requestAnimationFrame(this.animateGestureTrail_.bind(this));
};


/**
 * Returns a gesture point for a given event, with the correct coordinates.
 *
 * @param {!i18n.input.chrome.inputview.events.DragEvent|
 *     i18n.input.chrome.inputview.events.PointerEvent} e The event to
 *         convert.
 * @return {!Point} The converted gesture point.
 * @private
 */
GestureCanvasView.prototype.createGesturePoint_ = function(e) {
  var offset = goog.style.getPageOffset(this.drawingCanvas_);
  return new Point(e.x - offset.x, e.y - offset.y, e.identifier);
};


/**
 * Degrades the ttl of the points in all gesture strokes.
 *
 * @private
 */
GestureCanvasView.prototype.degradeStrokes_ = function() {
  for (var i = 0; i < this.strokeList_.length; i++) {
    this.strokeList_[i].degrade();
  }
};
});  // goog.scope
