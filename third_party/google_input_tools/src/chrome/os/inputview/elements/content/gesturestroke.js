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
/**
 * @fileoverview A single gesture typing stroke on the canvas.
 */
goog.provide('i18n.input.chrome.inputview.elements.content.GestureStroke');

goog.require('i18n.input.chrome.inputview.elements.content.Point');

goog.scope(function() {
var Point = i18n.input.chrome.inputview.elements.content.Point;



/**
 * @constructor
 */
i18n.input.chrome.inputview.elements.content.GestureStroke =
    function() {
  /**
   * The list of points that make up this stroke.
   *
   * @private {!Array.<!Point>}
   */
  this.points_ = [];

  /**
   * Whether or not this stroke is considered active. i.e. whether or not it
   * should be considered for rendering and decoding.
   *
   * @private {boolean}
   */
  this.isActive_ = false;

  /**
   * The time the first point was added to this stroke. Used to keep all points
   * relative to the first point.
   *
   * @private {number}
   */
  this.firstTime_ = 0;
};
var GestureStroke =
    i18n.input.chrome.inputview.elements.content.GestureStroke;


/**
 * Rate at which the ttl should degrade for the fading stroke effect.
 *
 * @const {number}
 * @private
 */
GestureStroke.DEGRADATION_RATE_ = 7;


/**
 * Distance threshold for when a stroke is considered active, in pixels.
 * TODO(stevet): This is temporary and needs to be updated with a dynamic value
 * that considers other parameters like the width of character keys.
 *
 * @const {number}
 * @private
 */
GestureStroke.ACTIVE_THRESHOLD_ = 40;


/**
 * Starting red value.
 *
 * @const {number}
 * @private
 */
GestureStroke.STARTING_R_VALUE_ = 68;


/**
 * Starting green value.
 *
 * @const {number}
 * @private
 */
GestureStroke.STARTING_G_VALUE_ = 132;


/**
 * Starting blue value.
 *
 * @const {number}
 * @private
 */
GestureStroke.STARTING_B_VALUE_ = 244;


/**
 * @const {number}
 * @private
 */
GestureStroke.STROKE_MAX_WIDTH_ = 9;


/**
 * Returns a string that represents the color of the point based on the ttl in
 * a format usable as a canvas stroke style.
 *
 * @param {number} ttl The time to live of the point.
 * @return {string} The color to use for the point.
 * @private
 */
GestureStroke.calculateColor_ = function(ttl) {
  // TODO(maxw): Use this percentage to fade the stroke color.
  var remainingTtlPercentage = ttl / Point.STARTING_TTL;
  var rValue = GestureStroke.STARTING_R_VALUE_;
  var gValue = GestureStroke.STARTING_G_VALUE_;
  var bValue = GestureStroke.STARTING_B_VALUE_;
  return 'rgb(' + rValue + ', ' + gValue + ', ' + bValue + ')';
};


/**
 * Calculates the distance between two Points using the Pythagorean theorem.
 * Note that goog.math.Coordinate provides a distance method, but requires that
 * points are passed in as Coordinate objects. By defining a simple
 * implementation here, we avoid the cost of creating Coordinate objects
 * repeatedly.
 *
 * @param {!Point} first The first point.
 * @param {!Point} second The second point.
 * @return {number} The number of pixels between first and second.
 * @private
 */
GestureStroke.calculateDistance_ = function(first, second) {
  var a = Math.abs(first.x - second.x);
  var b = Math.abs(first.y - second.y);
  return Math.sqrt(Math.pow(a, 2) + Math.pow(b, 2));
};


/**
 * Calculates the line width of the point based on the ttl.
 *
 * @param {number} ttl The time to live of the point.
 * @return {number} The line width to use for the point.
 * @private
 */
GestureStroke.calculateLineWidth_ = function(ttl) {
  var ratio = ttl / Point.STARTING_TTL;
  if (ratio < 0) {
    ratio = 0;
  }
  // Reduce the max width proportionately with the current ratio.
  return GestureStroke.STROKE_MAX_WIDTH_ * ratio;
};


/**
 * Degrades all the points in this stroke.
 *
 * @return {boolean} Returns true if it was possible to degrade one or more
 *     points, otherwise it means that this stroke is now empty.
 */
GestureStroke.prototype.degrade = function() {
  var allEmpty = true;
  for (var i = 0; i < this.points_.length; i++) {
    if (this.points_[i].ttl > 0) {
      this.points_[i].ttl -= GestureStroke.DEGRADATION_RATE_;
      allEmpty = false;
    }
  }
  return !allEmpty;
};


/**
 * Draw the gesture trail for this stroke onto the canvas context.
 *
 * @param {!CanvasRenderingContext2D} context The drawing context to render to.
 */
GestureStroke.prototype.draw = function(context) {
  // Only start drawing active strokes. Note that TTL still updates even if a
  // stroke is not yet active.
  if (!this.isActive()) {
    return;
  }

  for (var i = 1; i < this.points_.length; i++) {
    var first = this.points_[i - 1];
    var second = this.points_[i];
    // All rendering calculations are based on the second point in the segment
    // because there must be at least two points for something to be rendered.
    var ttl = second.ttl;
    if (ttl <= 0) {
      continue;
    }

    context.beginPath();
    context.moveTo(first.x, first.y);
    context.lineTo(second.x, second.y);
    context.strokeStyle = GestureStroke.calculateColor_(ttl);
    context.fillStyle = 'none';
    context.lineWidth = GestureStroke.calculateLineWidth_(ttl);
    context.lineCap = 'round';
    context.lineJoin = 'round';
    context.stroke();
  }
};


/**
 * @param {number} i The index to look up.
 * @return {number} The identifier of the point at index i.
 */
GestureStroke.prototype.getIdentifierAt = function(i) {
  return this.points_[i].identifier;
};


/**
 * @param {number} i The index to look up.
 * @return {number} The time of the point at index i.
 */
GestureStroke.prototype.getTimeAt = function(i) {
  return this.points_[i].time;
};


/**
 * @return {!Array.<!Point>} The points stored in the stroke.
 */
GestureStroke.prototype.getPoints = function() {
  return this.points_;
};


/**
 * Returns true iff this stroke is considered "active". This means that it has
 * passed a certain threshold and should be considered for rendering and
 * decoding.
 *
 * @return {boolean} Whether or not the stroke is active.
 */
GestureStroke.prototype.isActive = function() {
  // Once a stroke is active, it remains active.
  if (this.isActive_) {
    return this.isActive_;
  }

  if (this.points_.length < 2) {
    return false;
  }

  // Calculate the distance between the first point and the latest one.
  this.isActive_ = GestureStroke.calculateDistance_(
      this.points_[0], this.points_[this.points_.length - 1]) >
          GestureStroke.ACTIVE_THRESHOLD_;

  return this.isActive_;
};


/**
 * @return {boolean} Whether or not the stroke is degraded.
 */
GestureStroke.prototype.isDegraded = function() {
  var allPointsDegraded = true;
  for (var i = 0; i < this.points_.length; i++) {
    if (this.points_[i].ttl > 0) {
      allPointsDegraded = false;
      break;
    }
  }
  return allPointsDegraded;
};


/**
 * @return {number} The length of the stroke, which is the number of points it
 *     contains.
 */
GestureStroke.prototype.length = function() {
  return this.points_.length;
};


/**
 * Add a point to this stroke.
 *
 * @param {!Point} p The point to add to this stroke.
 */
GestureStroke.prototype.pushPoint = function(p) {
  if (this.points_.length == 0) {
    this.firstTime_ = p.time;
  }
  // Convert the timestamp so it is relative to the first point, including
  // setting the first point to zero.
  p.time -= this.firstTime_;
  this.points_.push(p);
};
});  // goog.scope
