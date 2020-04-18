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

/**
 * @fileoverview Handwriting input canvas.
 * @author har@google.com (Henry A. Rowley)
 */

goog.provide('i18n.input.hwt.Canvas');

goog.require('goog.a11y.aria.Announcer');
goog.require('goog.a11y.aria.LivePriority');
goog.require('goog.dom.TagName');
goog.require('goog.dom.classlist');
goog.require('goog.events.Event');
goog.require('goog.events.EventHandler');
goog.require('goog.events.EventType');
goog.require('goog.log');
goog.require('goog.ui.Container');
goog.require('i18n.input.hwt.EventType');
goog.require('i18n.input.hwt.StrokeHandler');
goog.require('i18n.input.hwt.css');



/**
 * The handwriting canvas UI element.
 *
 * @constructor
 * @param {!Document=} opt_topDocument The top document for MOUSEUP event.
 * @param {goog.dom.DomHelper=} opt_domHelper Optional DOM helper.
 * @param {goog.events.EventTarget=} opt_eventTarget The event target.
 * @param {number=} opt_inkWidth The ink width.
 * @param {string=} opt_inkColor The ink color.
 * @extends {goog.ui.Container}
 */
i18n.input.hwt.Canvas = function(opt_topDocument, opt_domHelper,
    opt_eventTarget, opt_inkWidth, opt_inkColor) {
  i18n.input.hwt.Canvas.base(this, 'constructor', undefined, undefined,
                             opt_domHelper);
  this.setParentEventTarget(opt_eventTarget || null);

  /**
   * The stroke handler.
   *
   * @type {!i18n.input.hwt.StrokeHandler}
   * @private
   */
  this.strokeHandler_;

  /**
   * The top document.
   *
   * @type {!Document}
   * @private
   */
  this.topDocument_ = opt_topDocument || document;

  /**
   * Canvas which is the area showing the ink.  Note that since we're
   * using a canvas here probably this code won't work in IE.
   *
   * @type {!Element}
   * @private
   */
  this.writingCanvas_;

  /**
   * Context for drawing into the writing canvas.
   *
   * @type {!CanvasRenderingContext2D}
   * @private
   */
  this.writingContext_;

  /**
   * An array of strokes for passing to the recognizer.
   *
   * @type {!Array}
   */
  this.strokeList = [];

  /**
   * The current stroke.
   *
   * @type {!Array}
   * @private
   */
  this.stroke_ = [];

  /**
   * Starting time of the stroke in milliseconds.
   *
   * @type {number}
   * @private
   */
  this.startTime_ = 0;

  /**
   * Event handler.
   *
   * @type {!goog.events.EventHandler}
   * @private
   */
  this.handler_ = new goog.events.EventHandler(this);

  /**
   * The annoucer for screen reader.
   *
   * @type {!goog.a11y.aria.Announcer}
   * @private
   */
  this.announcer_ = new goog.a11y.aria.Announcer(this.getDomHelper());

  /**
   * Width of the ink line.
   *
   * @type {number}
   * @private
   */
  this.inkWidth_ = opt_inkWidth || 6;

  /**
   * Color of the ink before it has been recognized.
   *
   * @type {string}
   * @private
   */
  this.inkColor_ = opt_inkColor || '#4D90FE';
};
goog.inherits(i18n.input.hwt.Canvas, goog.ui.Container);


/**
 * The class logger.
 *
 * @type {goog.log.Logger}
 * @private
 */
i18n.input.hwt.Canvas.prototype.logger_ =
    goog.log.getLogger('i18n.input.hwt.Canvas');


/**
 * @desc Label for handwriting panel used to draw strokes.
 */
i18n.input.hwt.Canvas.MSG_INPUTTOOLS_HWT_PANEL = goog.getMsg('panel');


/**
 * @desc The hint on the canvas to indicate users they can draw here.
 */
i18n.input.hwt.Canvas.MSG_HANDWRITING_HINT = goog.getMsg('Draw a symbol here');


/** @override */
i18n.input.hwt.Canvas.prototype.createDom = function() {
  i18n.input.hwt.Canvas.base(this, 'createDom');

  var dom = this.getDomHelper();
  this.writingCanvas_ = dom.createDom(goog.dom.TagName.CANVAS,
      i18n.input.hwt.css.CANVAS);
  this.writingCanvas_.width = 425;
  this.writingCanvas_.height = 194;
  dom.appendChild(this.getElement(), this.writingCanvas_);
  this.writingContext_ = this.writingCanvas_.getContext('2d');
};


/** @override */
i18n.input.hwt.Canvas.prototype.enterDocument = function() {
  i18n.input.hwt.Canvas.base(this, 'enterDocument');

  this.setFocusable(false);
  this.setFocusableChildrenAllowed(false);
  // Sets up stroke handler.
  this.strokeHandler_ = new i18n.input.hwt.StrokeHandler(this.writingCanvas_,
      this.topDocument_);
  this.handler_.
      listen(this.strokeHandler_,
          i18n.input.hwt.StrokeHandler.EventType.STROKE_START,
          this.onStrokeStart_).
      listen(this.strokeHandler_,
          i18n.input.hwt.StrokeHandler.EventType.STROKE,
          this.onStroke_).
      listen(this.strokeHandler_,
          i18n.input.hwt.StrokeHandler.EventType.STROKE_END,
          this.onStrokeEnd_).
      listen(this.writingCanvas_, goog.events.EventType.MOUSEOVER,
          this.onMouseOver_).
      listen(this.writingCanvas_, goog.events.EventType.MOUSEDOWN,
          goog.events.Event.stopPropagation);
};


/**
 * Shows the hint message for the canvas.
 */
i18n.input.hwt.Canvas.prototype.showHint = function() {
  this.writingContext_.font = '20px aria,sans-serif';
  this.writingContext_.fontWeight = 'bold';
  this.writingContext_.fillStyle = '#CCC';
  this.writingContext_.fillText(i18n.input.hwt.Canvas.MSG_HANDWRITING_HINT,
      30, 120);
};


/**
 * Draw a point with the given color.
 *
 * @param {string} color Color to draw the point.
 * @param {!i18n.input.hwt.StrokeHandler.Point} point The point.
 * @private
 */
i18n.input.hwt.Canvas.prototype.drawPoint_ = function(color, point) {
  this.writingContext_.beginPath();
  this.writingContext_.strokeStyle = color;
  this.writingContext_.fillStyle = color;
  this.writingContext_.arc(point.x, point.y,
      this.inkWidth_ / 2, 0, Math.PI * 2, true);
  this.writingContext_.fill();
};


/**
 * Draw a poly-line given the list of array of points.
 *
 * @param {string} color Color to draw the line.
 * @param {!Array.<!i18n.input.hwt.StrokeHandler.Point>} points The array of
 *     points.
 * @param {number=} opt_start The start point to draw.
 * @private
 */
i18n.input.hwt.Canvas.prototype.drawLine_ = function(color, points, opt_start) {
  var start = opt_start || 0;
  if (start == (points.length - 1)) {
    // If there's only one point.
    this.drawPoint_(color, points[0]);
  } else {
    this.writingContext_.beginPath();
    this.writingContext_.strokeStyle = color;
    this.writingContext_.fillStyle = 'none';
    this.writingContext_.lineWidth = this.inkWidth_;
    this.writingContext_.lineCap = 'round';
    this.writingContext_.lineJoin = 'round';
    this.writingContext_.moveTo(points[start].x, points[start].y);
    for (var i = start + 1; i < points.length; i++) {
      this.writingContext_.lineTo(points[i].x, points[i].y);
    }
    this.writingContext_.stroke();
  }
};


/**
 * Add a point to the current stroke.
 *
 * @param {!i18n.input.hwt.StrokeHandler.Point} point The point.
 * @private
 */
i18n.input.hwt.Canvas.prototype.addPoint_ = function(point) {
  point.time -= this.startTime_;
  this.stroke_.push(point);
  var len = this.stroke_.length;
  var start = Math.max(len - 2, 0);
  this.drawLine_(this.inkColor_, this.stroke_, start);
};


/**
 * Callback for the start of a stroke.
 *
 * @param {!i18n.input.hwt.StrokeHandler.StrokeEvent} e The stroke event.
 * @private
 */
i18n.input.hwt.Canvas.prototype.onStrokeStart_ = function(e) {
  this.stroke_ = [];
  var point = e.point;
  if (this.strokeList.length == 0) {
    this.startTime_ = point.time;
  }
  this.addPoint_(e.point);
  e.preventDefault();

  goog.dom.classlist.add(this.getElement(), i18n.input.hwt.css.IME_INIT_OPAQUE);
};


/**
 * Callback for the stroke event.
 *
 * @param {!i18n.input.hwt.StrokeHandler.StrokeEvent} e The stroke event.
 * @private
 */
i18n.input.hwt.Canvas.prototype.onStroke_ = function(e) {
  this.addPoint_(e.point);
  e.preventDefault();
};


/**
 * Callback for the end of a stroke.
 *
 * @param {i18n.input.hwt.StrokeHandler.StrokeEvent} e The stroke event.
 * @private
 */
i18n.input.hwt.Canvas.prototype.onStrokeEnd_ = function(e) {
  this.strokeList.push(this.stroke_);
  e.preventDefault();
  this.dispatchEvent(new goog.events.Event(
      i18n.input.hwt.EventType.RECOGNITION_READY));
};


/**
 * Clears the writing canvas.
 */
i18n.input.hwt.Canvas.prototype.reset = function() {
  goog.log.info(this.logger_, 'clear ' + this.writingCanvas_.width + 'x' +
      this.writingCanvas_.height);
  this.writingContext_.clearRect(
      0, 0, this.writingCanvas_.width, this.writingCanvas_.height);
  this.strokeList = [];
  this.stroke_ = [];
  this.strokeHandler_.reset();
};


/** @override */
i18n.input.hwt.Canvas.prototype.disposeInternal = function() {
  goog.dispose(this.handler_);
  i18n.input.hwt.Canvas.base(this, 'disposeInternal');
};

/**
 * Gets the width of the canvas.
 *
 * @return {number} The width of the canvas.
 */
i18n.input.hwt.Canvas.prototype.getWidth = function() {
  return this.writingCanvas_.width;
};


/**
 * Gets the height of the canvas.
 *
 * @return {number} The height of the canvas.
 */
i18n.input.hwt.Canvas.prototype.getHeight = function() {
  return this.writingCanvas_.height;
};


/**
 * True if user is drawing.
 *
 * @return {boolean} True if is drawing.
 */
i18n.input.hwt.Canvas.prototype.isDrawing = function() {
  return this.strokeHandler_.drawing;
};


/**
 * True if the canvas is clean without strokes.
 *
 * @return {boolean} True if the canvas is clean.
 */
i18n.input.hwt.Canvas.prototype.isClean = function() {
  return !this.strokeHandler_.drawing && this.strokeList.length == 0;
};


/**
 * Sets the size of the canvas.
 *
 * @param {number=} opt_height The height.
 * @param {number=} opt_width The width.
 */
i18n.input.hwt.Canvas.prototype.setSize = function(opt_height, opt_width) {
  if (opt_height && this.writingCanvas_.height != opt_height ||
      opt_width && this.writingCanvas_.width != opt_width) {
    this.reset();
  }
  if (opt_height) {
    this.writingCanvas_.height = opt_height;
  }
  if (opt_width) {
    this.writingCanvas_.width = opt_width;
  }
};


/**
 * Gets the stroke handler.
 *
 * @return {!i18n.input.hwt.StrokeHandler} The stroke handler.
 */
i18n.input.hwt.Canvas.prototype.getStrokeHandler = function() {
  return this.strokeHandler_;
};


/**
 * Exports message to screen reader.
 *
 * @param {!goog.events.BrowserEvent} e .
 * @private
 */
i18n.input.hwt.Canvas.prototype.onMouseOver_ = function(e) {
  this.announcer_.say(i18n.input.hwt.Canvas.MSG_INPUTTOOLS_HWT_PANEL,
      goog.a11y.aria.LivePriority.ASSERTIVE);
};
