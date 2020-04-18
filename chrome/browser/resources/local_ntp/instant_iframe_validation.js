// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


/**
 * @fileoverview Helpers for validating parameters to chrome-search:// iframes.
 */


/**
 * Converts an RGB color number to a hex color string if valid.
 * @param {number} color A 6-digit hex RGB color code as a number.
 * @return {?string} A CSS representation of the color or null if invalid.
 */
function convertToHexColor(color) {
  // Color must be a number, finite, with no fractional part, in the correct
  // range for an RGB hex color.
  if (isFinite(color) && Math.floor(color) == color && color >= 0 &&
      color <= 0xffffff) {
    var hexColor = color.toString(16);
    // Pads with initial zeros and # (e.g. for 'ff' yields '#0000ff').
    return '#000000'.substr(0, 7 - hexColor.length) + hexColor;
  }
  return null;
}


/**
 * Validates a RGBA color component. It must be a number between 0 and 255.
 * @param {number} component An RGBA component.
 * @return {boolean} True if the component is valid.
 */
function isValidRBGAComponent(component) {
  return isFinite(component) && component >= 0 && component <= 255;
}


/**
 * Converts an Array of color components into RGBA format "rgba(R,G,B,A)".
 * @param {Array<number>} rgbaColor Array of rgba color components.
 * @return {?string} CSS color in RGBA format or null if invalid.
 */
function convertArrayToRGBAColor(rgbaColor) {
  // Array must contain 4 valid components.
  if (rgbaColor instanceof Array && rgbaColor.length === 4 &&
      isValidRBGAComponent(rgbaColor[0]) &&
      isValidRBGAComponent(rgbaColor[1]) &&
      isValidRBGAComponent(rgbaColor[2]) &&
      isValidRBGAComponent(rgbaColor[3])) {
    return 'rgba(' + rgbaColor[0] + ',' + rgbaColor[1] + ',' + rgbaColor[2] +
        ',' + rgbaColor[3] / 255 + ')';
  }
  return null;
}
