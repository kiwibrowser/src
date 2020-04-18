// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var ImageLoaderUtil = {};

/**
 * Checks if the options contain any image processing.
 *
 * @param {number} width Source width.
 * @param {number} height Source height.
 * @param {Object} options Resizing options as a hash array.
 * @return {boolean} True if yes, false if not.
 */
ImageLoaderUtil.shouldProcess = function(width, height, options) {
  var targetDimensions = ImageLoaderUtil.resizeDimensions(
      width, height, options);

  // Dimensions has to be adjusted.
  if (targetDimensions.width != width || targetDimensions.height != height)
    return true;

  // Orientation has to be adjusted.
  if (!options.orientation.isIdentity())
    return true;

  // Non-standard color space has to be converted.
  if (options.colorSpace && options.colorSpace !== ColorSpace.SRGB)
    return true;

  // No changes required.
  return false;
};

/**
 * Calculates dimensions taking into account resize options, such as:
 * - scale: for scaling,
 * - maxWidth, maxHeight: for maximum dimensions,
 * - width, height: for exact requested size.
 * Returns the target size as hash array with width, height properties.
 *
 * @param {number} width Source width.
 * @param {number} height Source height.
 * @param {Object} options Resizing options as a hash array.
 * @return {Object} Dimensions, eg. {width: 100, height: 50}.
 */
ImageLoaderUtil.resizeDimensions = function(width, height, options) {
  var scale = options.scale || 1;
  var targetDimensions = options.orientation.getSizeAfterCancelling(
      width * scale, height * scale);
  var targetWidth = targetDimensions.width;
  var targetHeight = targetDimensions.height;

  if (options.maxWidth && targetWidth > options.maxWidth) {
    var scale = options.maxWidth / targetWidth;
    targetWidth *= scale;
    targetHeight *= scale;
  }

  if (options.maxHeight && targetHeight > options.maxHeight) {
    var scale = options.maxHeight / targetHeight;
    targetWidth *= scale;
    targetHeight *= scale;
  }

  if (options.width)
    targetWidth = options.width;

  if (options.height)
    targetHeight = options.height;

  targetWidth = Math.round(targetWidth);
  targetHeight = Math.round(targetHeight);

  return {width: targetWidth, height: targetHeight};
};

/**
 * Performs resizing and cropping of the source image into the target canvas.
 *
 * @param {HTMLCanvasElement|Image} source Source image or canvas.
 * @param {HTMLCanvasElement} target Target canvas.
 * @param {Object} options Resizing options as a hash array.
 */
ImageLoaderUtil.resizeAndCrop = function(source, target, options) {
  // Calculates copy parameters.
  var copyParameters = ImageLoaderUtil.calculateCopyParameters(
      source, options);
  target.width = copyParameters.canvas.width;
  target.height = copyParameters.canvas.height;

  // Apply.
  var targetContext =
      /** @type {CanvasRenderingContext2D} */ (target.getContext('2d'));
  targetContext.save();
  options.orientation.cancelImageOrientation(
      targetContext, copyParameters.target.width, copyParameters.target.height);
  targetContext.drawImage(
      source,
      copyParameters.source.x,
      copyParameters.source.y,
      copyParameters.source.width,
      copyParameters.source.height,
      copyParameters.target.x,
      copyParameters.target.y,
      copyParameters.target.width,
      copyParameters.target.height);
  targetContext.restore();
};

/**
 * @typedef {{
 *   source: {x:number, y:number, width:number, height:number},
 *   target: {x:number, y:number, width:number, height:number},
 *   canvas: {width:number, height:number}
 * }}
 */
ImageLoaderUtil.CopyParameters;

/**
 * Calculates copy parameters.
 *
 * @param {HTMLCanvasElement|Image} source Source image or canvas.
 * @param {Object} options Resizing options as a hash array.
 * @return {!ImageLoaderUtil.CopyParameters} Calculated copy parameters.
 */
ImageLoaderUtil.calculateCopyParameters = function(source, options) {
  if (options.crop) {
    // When an image is cropped, target should be a fixed size square.
    assert(options.width);
    assert(options.height);
    assert(options.width === options.height);

    // The length of shorter edge becomes dimension of cropped area in the
    // source.
    var cropSourceDimension = Math.min(source.width, source.height);

    return {
      source: {
        x: Math.floor((source.width / 2) - (cropSourceDimension / 2)),
        y: Math.floor((source.height / 2) - (cropSourceDimension / 2)),
        width: cropSourceDimension,
        height: cropSourceDimension
      },
      target: {
        x: 0,
        y: 0,
        width: options.width,
        height: options.height
      },
      canvas: {
        width: options.width,
        height: options.height
      }
    };
  }

  // Target dimension is calculated in the rotated(transformed) coordinate.
  var targetCanvasDimensions = ImageLoaderUtil.resizeDimensions(
      source.width, source.height, options);

  var targetDimensions = options.orientation.getSizeAfterCancelling(
      targetCanvasDimensions.width, targetCanvasDimensions.height);

  return {
    source: {
      x: 0,
      y: 0,
      width: source.width,
      height: source.height
    },
    target: {
      x: 0,
      y: 0,
      width: targetDimensions.width,
      height: targetDimensions.height
    },
    canvas: {
      width: targetCanvasDimensions.width,
      height: targetCanvasDimensions.height
    }
  };
};

/**
 * Matrix converts AdobeRGB color space into sRGB color space.
 * @const {!Array<number>}
 */
ImageLoaderUtil.MATRIX_FROM_ADOBE_TO_STANDARD = [
  1.39836, -0.39836, 0.00000,
  0.00000,  1.00000, 0.00000,
  0.00000, -0.04293, 1.04293
];

/**
 * Converts the canvas of color space into sRGB.
 * @param {HTMLCanvasElement} target Target canvas.
 * @param {ColorSpace} colorSpace Current color space.
 */
ImageLoaderUtil.convertColorSpace = function(target, colorSpace) {
  if (colorSpace === ColorSpace.SRGB)
    return;
  if (colorSpace === ColorSpace.ADOBE_RGB) {
    var matrix = ImageLoaderUtil.MATRIX_FROM_ADOBE_TO_STANDARD;
    var context = target.getContext('2d');
    var imageData = context.getImageData(0, 0, target.width, target.height);
    var data = imageData.data;
    for (var i = 0; i < data.length; i += 4) {
      // Scale to [0, 1].
      var adobeR = data[i] / 255;
      var adobeG = data[i + 1] / 255;
      var adobeB = data[i + 2] / 255;

      // Revert gannma transformation.
      adobeR = adobeR <= 0.0556 ? adobeR / 32 : Math.pow(adobeR, 2.2);
      adobeG = adobeG <= 0.0556 ? adobeG / 32 : Math.pow(adobeG, 2.2);
      adobeB = adobeB <= 0.0556 ? adobeB / 32 : Math.pow(adobeB, 2.2);

      // Convert color space.
      var sR = matrix[0] * adobeR + matrix[1] * adobeG + matrix[2] * adobeB;
      var sG = matrix[3] * adobeR + matrix[4] * adobeG + matrix[5] * adobeB;
      var sB = matrix[6] * adobeR + matrix[7] * adobeG + matrix[8] * adobeB;

      // Gannma transformation.
      sR = sR <= 0.0031308 ? 12.92 * sR : 1.055 * Math.pow(sR, 1 / 2.4) - 0.055;
      sG = sG <= 0.0031308 ? 12.92 * sG : 1.055 * Math.pow(sG, 1 / 2.4) - 0.055;
      sB = sB <= 0.0031308 ? 12.92 * sB : 1.055 * Math.pow(sB, 1 / 2.4) - 0.055;

      // Scale to [0, 255].
      data[i] = Math.max(0, Math.min(255, sR * 255));
      data[i + 1] = Math.max(0, Math.min(255, sG * 255));
      data[i + 2] = Math.max(0, Math.min(255, sB * 255));
    }
    context.putImageData(imageData, 0, 0);
  }
};