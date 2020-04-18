// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

/**
 * The time in milliseconds of the animation updates.
 * @type {number}
 */
const ANIMATE_TICKS_MS = 20;

/**
 * The duration in milliseconds of the animation of the progress circle when the
 * user is touching the scanner.
 * @type {number}
 */
const ANIMATE_DURATION_MS = 200;

/**
 * The radius of the add fingerprint progress circle.
 * @type {number}
 */
const CANVAS_CIRCLE_RADIUS = 50;

/**
 * The thickness of the add fingerprint progress circle.
 * @type {number}
 */
const CANVAS_CIRCLE_STROKE_WIDTH = 4;

/**
 * The color of the canvas circle background.
 * @type {string}
 */
const CANVAS_CIRCLE_BACKGROUND_COLOR = 'rgba(66, 66, 66, 1.0)';

/**
 * The color of the arc/circle which indicates setup progress.
 * @type {string}
 */
const CANVAS_CIRCLE_PROGRESS_COLOR = 'rgba(53, 103, 214, 1.0)';

/**
 * The color of the canvas circle shadow.
 * @type {string}
 */
const CANVAS_CIRCLE_SHADOW_COLOR = 'rgba(0, 0, 0, 0.5)';

Polymer({
  is: 'settings-fingerprint-progress-arc',

  // Also put these values as member values so they can be overriden by tests
  // and the tests do not need to be changed every time the UI is.
  /** @private {number} */
  canvasCircleRadius_: CANVAS_CIRCLE_RADIUS,
  /** @private {number} */
  canvasCircleStrokeWidth_: CANVAS_CIRCLE_STROKE_WIDTH,
  /** @private {string} */
  canvasCircleBackgroundColor_: CANVAS_CIRCLE_BACKGROUND_COLOR,
  /** @private {string} */
  canvasCircleProgressColor_: CANVAS_CIRCLE_PROGRESS_COLOR,
  /** @private {string} */
  canvasCircleShadowColor_: CANVAS_CIRCLE_SHADOW_COLOR,

  /**
   * Draws an arc on the canvas element around the center with radius
   * |CANVAS_CIRCLE_RADIUS|.
   * @param {number} startAngle The start angle of the arc we want to draw.
   * @param {number} endAngle The end angle of the arc we want to draw.
   * @param {string} color The color of the arc we want to draw. The string is
   *     in the format rgba(r',g',b',a'). r', g', b' are values from [0-255]
   *     and a' is a value from [0-1].
   */
  drawArc: function(startAngle, endAngle, color) {
    const c = this.$.canvas;
    const ctx = c.getContext('2d');

    ctx.beginPath();
    ctx.arc(
        c.width / 2, c.height / 2, this.canvasCircleRadius_, startAngle,
        endAngle);
    ctx.lineWidth = this.canvasCircleStrokeWidth_;
    ctx.strokeStyle = color;
    ctx.stroke();
  },

  /**
   * Draws a circle on the canvas element around the center with radius
   * |CANVAS_CIRCLE_RADIUS| and color |CANVAS_CIRCLE_BACKGROUND_COLOR|.
   */
  drawBackgroundCircle: function() {
    this.drawArc(0, 2 * Math.PI, this.canvasCircleBackgroundColor_);
  },

  /**
   * Draws a circular shadow around the center with radius
   * |CANVAS_CIRCLE_RADIUS|.
   * @param {number} blur
   * @param {number} offsetX
   * @param {number} offsetY
   */
  drawShadow: function(blur, offsetX, offsetY) {
    const c = this.$.canvas;
    const ctx = c.getContext('2d');

    ctx.beginPath();
    ctx.translate(-c.width, 0);
    ctx.shadowOffsetX = c.width + offsetX;
    ctx.shadowOffsetY = 0;
    ctx.shadowColor = this.canvasCircleShadowColor_;
    ctx.shadowBlur = blur;
    ctx.arc(
        c.width / 2, c.height / 2,
        this.canvasCircleRadius_ - this.canvasCircleStrokeWidth_ / 2 + blur / 2,
        0, 2 * Math.PI);
    ctx.stroke();
    ctx.translate(c.width, 0);
  },

  /**
   * Animates the progress the circle. Animates an arc that starts at the top of
   * the circle to startAngle, to an arc that starts at the top of the circle to
   * endAngle.
   * @param {number} startAngle The start angle of the arc we want to draw.
   * @param {number} endAngle The end angle of the arc we want to draw.
   */
  animateProgress: function(startAngle, endAngle) {
    let currentAngle = startAngle;
    // The value to update the angle by each tick.
    const step =
        (endAngle - startAngle) / (ANIMATE_DURATION_MS / ANIMATE_TICKS_MS);
    const id = setInterval(doAnimate.bind(this), ANIMATE_TICKS_MS);
    // Circles on html canvas have 0 radians on the positive x-axis and go in
    // clockwise direction. We want to start at the top of the circle which is
    // 3pi/2.
    const start = 3 * Math.PI / 2;

    // Function that is called every tick of the interval, draws the arc a bit
    // closer to the final destination each tick, until it reaches the final
    // destination.
    function doAnimate() {
      if (currentAngle >= endAngle)
        clearInterval(id);

      // Clears the canvas and draws the new progress circle.
      this.clearCanvas();
      // Drawing two arcs to form a circle gives a nicer look than drawing an
      // arc on top of a circle. If |currentAngle| is 0, draw from |start| +
      // |currentAngle| to 7 * Math.PI / 2 (start is 3 * Math.PI / 2) otherwise
      // the regular draw from |start| to |currentAngle| will draw nothing which
      // will cause a flicker for one frame.
      this.drawArc(
          start, start + currentAngle, this.canvasCircleProgressColor_);
      this.drawArc(
          start + currentAngle, currentAngle <= 0 ? 7 * Math.PI / 2 : start,
          this.canvasCircleBackgroundColor_);
      currentAngle += step;
    }
  },

  /**
   * Clear the canvas of any renderings.
   */
  clearCanvas: function() {
    const c = this.$.canvas;
    const ctx = c.getContext('2d');
    ctx.clearRect(0, 0, c.width, c.height);
  },
});
})();
