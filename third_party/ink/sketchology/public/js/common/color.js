// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
goog.provide('ink.Color');



/**
 * 32bit color representation. Each channels is a 8 bit uint.
 * @param {number} argb The 32-bit color.
 * @constructor
 * @struct
 */
ink.Color = function(argb) {
  /** @type {number} */
  this.argb = argb;

  /** @type {number} */
  this.a = ink.Color.alphaFromArgb(argb);

  /** @type {number} */
  this.r = ink.Color.redFromArgb(argb);

  /** @type {number} */
  this.g = ink.Color.greenFromArgb(argb);

  /** @type {number} */
  this.b = ink.Color.blueFromArgb(argb);
};


/**
 * @return {string} The color string (excluding alpha) that can be used as a
 *     fillStyle.
 */
ink.Color.prototype.getRgbString = function() {
  return 'rgb(' + [this.r, this.g, this.b].join(',') + ')';
};


/** @return {string} The color string that can be used as a fillStyle. */
ink.Color.prototype.getRgbaString = function() {
  return 'rgba(' + [this.r, this.g, this.b, this.a / 255].join(',') + ')';
};


/** @return {Uint32Array} color as rgba 32-bit unsigned integer */
ink.Color.prototype.getRgbaUint32 = function() {
  return new Uint32Array(
      [(this.r << 24) | (this.g << 16) | (this.b << 8) | this.a]);
};


/**
 * @return {number} The alpha in the range 0-1 that can be used as a
 *     globalAlpha.
 */
ink.Color.prototype.getAlphaAsFloat = function() {
  return this.a / 255;
};


/**
 * Helper function that returns a function that right logical shifts by the
 * provided amount and masks off the result.
 * @param {number} shiftAmount The amount that the function should shift by.
 * @return {!Function}
 * @private
 */
ink.Color.shiftAndMask_ = function(shiftAmount) {
  return function(argb) {
    return (argb >>> shiftAmount) & 0xFF;
  };
};


/**
 * @param {number} argb The argb number.
 * @return {number} alpha in the range 0 to 255.
 */
ink.Color.alphaFromArgb = ink.Color.shiftAndMask_(24);


/**
 * @param {number} argb The argb number.
 * @return {number} red in the range 0 to 255.
 */
ink.Color.redFromArgb = ink.Color.shiftAndMask_(16);


/**
 * @param {number} argb The argb number.
 * @return {number} green in the range 0 to 255.
 */
ink.Color.greenFromArgb = ink.Color.shiftAndMask_(8);


/**
 * @param {number} argb The argb number.
 * @return {number} blue in the range 0 to 255.
 */
ink.Color.blueFromArgb = ink.Color.shiftAndMask_(0);


/** @type {!ink.Color} */
ink.Color.BLACK = new ink.Color(0xFF000000);


/** @type {!ink.Color} */
ink.Color.WHITE = new ink.Color(0xFFFFFFFF);

/** @type {!ink.Color} */
ink.Color.DEFAULT_BACKGROUND_COLOR = new ink.Color(0xFFFAFAFA);
