// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * A namespace for image filter utilities.
 */
var filter = {};

/**
 * Create a filter from name and options.
 *
 * @param {string} name Maps to a filter method name.
 * @param {!Object} options A map of filter-specific options.
 * @return {function(!ImageData,!ImageData,number,number)} created function.
 */
filter.create = function(name, options) {
  var filterFunc = filter[name](options);
  return function() {
    var time = Date.now();
    filterFunc.apply(null, arguments);
    var dst = arguments[0];
    var mPixPerSec = dst.width * dst.height / 1000 / (Date.now() - time);
    ImageUtil.trace.report(name, Math.round(mPixPerSec * 10) / 10 + 'Mps');
  }
};

/**
 * Apply a filter to a image by splitting it into strips.
 *
 * To be used with large images to avoid freezing up the UI.
 *
 * @param {!HTMLCanvasElement} dstCanvas Destination canvas.
 * @param {!HTMLCanvasElement|!HTMLImageElement} srcImage Source image.
 * @param {function(!ImageData,!ImageData,number,number)} filterFunc Filter.
 * @param {function(number, number)} progressCallback Progress callback.
 * @param {number=} opt_maxPixelsPerStrip Pixel number to process at once.
 */
filter.applyByStrips = function(
    dstCanvas, srcImage, filterFunc, progressCallback, opt_maxPixelsPerStrip) {
  // 1 Mpix is a reasonable default.
  var maxPixelsPerStrip = opt_maxPixelsPerStrip || 1000000;

  var dstContext = dstCanvas.getContext('2d');
  var srcContext = ImageUtil.ensureCanvas(srcImage).getContext('2d');
  var source = srcContext.getImageData(0, 0, srcImage.width, srcImage.height);

  var stripCount = Math.ceil(srcImage.width * srcImage.height /
      maxPixelsPerStrip);

  var strip = srcContext.getImageData(0, 0,
      srcImage.width, Math.ceil(srcImage.height / stripCount));

  var offset = 0;

  function filterStrip() {
    // If the strip overlaps the bottom of the source image we cannot shrink it
    // and we cannot fill it partially (since canvas.putImageData always draws
    // the entire buffer).
    // Instead we move the strip up several lines (converting those lines
    // twice is a small price to pay).
    if (offset > source.height - strip.height) {
      offset = source.height - strip.height;
    }

    filterFunc(strip, source, 0, offset);
    dstContext.putImageData(strip, 0, offset);

    offset += strip.height;

    if (offset < source.height) {
      setTimeout(filterStrip, 0);
    } else {
      ImageUtil.trace.reportTimer('filter-commit');
    }

    progressCallback(offset, source.height);
  }

  ImageUtil.trace.resetTimer('filter-commit');
  filterStrip();
};

/**
 * Return a color histogram for an image.
 *
 * @param {!(HTMLCanvasElement|ImageData)} source Image data to analyze.
 * @return {{r: !Array<number>, g: !Array<number>, b: !Array<number>}}
 *     histogram.
 */
filter.getHistogram = function(source) {
  var imageData;
  if (source.constructor.name == 'HTMLCanvasElement') {
    imageData = source.getContext('2d').
        getImageData(0, 0, source.width, source.height);
  } else {
    imageData = source;
  }

  var r = [];
  var g = [];
  var b = [];

  for (var i = 0; i != 256; i++) {
    r.push(0);
    g.push(0);
    b.push(0);
  }

  var data = imageData.data;
  var maxIndex = 4 * imageData.width * imageData.height;
  for (var index = 0; index != maxIndex;) {
    r[data[index++]]++;
    g[data[index++]]++;
    b[data[index++]]++;
    index++;
  }

  return { r: r, g: g, b: b };
};

/**
 * Compute the function for every integer value from 0 up to maxArg.
 *
 * Rounds and clips the results to fit the [0..255] range.
 * Useful to speed up pixel manipulations.
 *
 * @param {number} maxArg Maximum argument value (inclusive).
 * @param {function(number): number} func Function to precompute.
 * @return {!Uint8Array} Computed results.
 */
filter.precompute = function(maxArg, func) {
  var results = new Uint8Array(maxArg + 1);
  for (var arg = 0; arg <= maxArg; arg++) {
    results[arg] = Math.max(0, Math.min(0xFF, Math.round(func(arg))));
  }
  return results;
};

/**
 * Convert pixels by applying conversion tables to each channel individually.
 *
 * @param {!Uint8Array} rMap Red channel conversion table.
 * @param {!Uint8Array} gMap Green channel conversion table.
 * @param {!Uint8Array} bMap Blue channel conversion table.
 * @param {!ImageData} dst Destination image data. Can be smaller than the
 *     source, must completely fit inside the source.
 * @param {!ImageData} src Source image data.
 * @param {!number} offsetX Horizontal offset of dst relative to src.
 * @param {!number} offsetY Vertical offset of dst relative to src.
 */
filter.mapPixels = function(rMap, gMap, bMap, dst, src, offsetX, offsetY) {
  var dstData = dst.data;
  var dstWidth = dst.width;
  var dstHeight = dst.height;

  var srcData = src.data;
  var srcWidth = src.width;
  var srcHeight = src.height;

  if (offsetX < 0 || offsetX + dstWidth > srcWidth ||
      offsetY < 0 || offsetY + dstHeight > srcHeight)
    throw new Error('Invalid offset');

  var dstIndex = 0;
  for (var y = 0; y != dstHeight; y++) {
    var srcIndex = (offsetX + (offsetY + y) * srcWidth) * 4;
    for (var x = 0; x != dstWidth; x++) {
      dstData[dstIndex++] = rMap[srcData[srcIndex++]];
      dstData[dstIndex++] = gMap[srcData[srcIndex++]];
      dstData[dstIndex++] = bMap[srcData[srcIndex++]];
      dstIndex++;
      srcIndex++;
    }
  }
};

/**
 * Number of digits after period(in binary form) to preserve.
 * @type {number}
 * @const
 */
filter.FIXED_POINT_SHIFT = 16;

/**
 * Maximum value that can be represented in fixed point without overflow.
 * @type {number}
 * @const
 */
filter.MAX_FLOAT_VALUE = 0x7FFFFFFF >> filter.FIXED_POINT_SHIFT;

/**
 * Converts floating point to fixed.
 * @param {number} x Number to convert.
 * @return {number} Converted number.
 */
filter.floatToFixedPoint = function(x) {
  // Math.round on negative arguments causes V8 to deoptimize the calling
  // function, so we are using >> 0 instead.
  return (x * (1 << filter.FIXED_POINT_SHIFT)) >> 0;
};

/**
 * Perform an image convolution with a symmetrical 5x5 matrix:
 *
 *  0  0 w3  0  0
 *  0 w2 w1 w2  0
 * w3 w1 w0 w1 w3
 *  0 w2 w1 w2  0
 *  0  0 w3  0  0
 *
 * @param {!Array<number>} weights See the picture above.
 * @param {!ImageData} dst Destination image data. Can be smaller than the
 *     source, must completely fit inside the source.
 * @param {!ImageData} src Source image data.
 * @param {number} offsetX Horizontal offset of dst relative to src.
 * @param {number} offsetY Vertical offset of dst relative to src.
 */
filter.convolve5x5 = function(weights, dst, src, offsetX, offsetY) {
  var w0 = filter.floatToFixedPoint(weights[0]);
  var w1 = filter.floatToFixedPoint(weights[1]);
  var w2 = filter.floatToFixedPoint(weights[2]);
  var w3 = filter.floatToFixedPoint(weights[3]);

  var dstData = dst.data;
  var dstWidth = dst.width;
  var dstHeight = dst.height;
  var dstStride = dstWidth * 4;

  var srcData = src.data;
  var srcWidth = src.width;
  var srcHeight = src.height;
  var srcStride = srcWidth * 4;
  var srcStride2 = srcStride * 2;

  if (offsetX < 0 || offsetX + dstWidth > srcWidth ||
      offsetY < 0 || offsetY + dstHeight > srcHeight)
    throw new Error('Invalid offset');

  // Javascript is not very good at inlining constants.
  // We inline manually and assert that the constant is equal to the variable.
  if (filter.FIXED_POINT_SHIFT != 16)
    throw new Error('Wrong fixed point shift');

  var margin = 2;

  var startX = Math.max(0, margin - offsetX);
  var endX = Math.min(dstWidth, srcWidth - margin - offsetX);

  var startY = Math.max(0, margin - offsetY);
  var endY = Math.min(dstHeight, srcHeight - margin - offsetY);

  for (var y = startY; y != endY; y++) {
    var dstIndex = y * dstStride + startX * 4;
    var srcIndex = (y + offsetY) * srcStride + (startX + offsetX) * 4;

    for (var x = startX; x != endX; x++) {
      for (var c = 0; c != 3; c++) {
        var sum = w0 * srcData[srcIndex] +
                  w1 * (srcData[srcIndex - 4] +
                        srcData[srcIndex + 4] +
                        srcData[srcIndex - srcStride] +
                        srcData[srcIndex + srcStride]) +
                  w2 * (srcData[srcIndex - srcStride - 4] +
                        srcData[srcIndex + srcStride - 4] +
                        srcData[srcIndex - srcStride + 4] +
                        srcData[srcIndex + srcStride + 4]) +
                  w3 * (srcData[srcIndex - 8] +
                        srcData[srcIndex + 8] +
                        srcData[srcIndex - srcStride2] +
                        srcData[srcIndex + srcStride2]);
        if (sum < 0)
          dstData[dstIndex++] = 0;
        else if (sum > 0xFF0000)
          dstData[dstIndex++] = 0xFF;
        else
          dstData[dstIndex++] = sum >> 16;
        srcIndex++;
      }
      srcIndex++;
      dstIndex++;
    }
  }
};

/**
 * Compute the average color for the image.
 *
 * @param {!ImageData} imageData Image data to analyze.
 * @return {{r: number, g: number, b: number}} average color.
 */
filter.getAverageColor = function(imageData) {
  var data = imageData.data;
  var width = imageData.width;
  var height = imageData.height;

  var total = 0;
  var r = 0;
  var g = 0;
  var b = 0;

  var maxIndex = 4 * width * height;
  for (var i = 0; i != maxIndex;) {
    total++;
    r += data[i++];
    g += data[i++];
    b += data[i++];
    i++;
  }
  if (total == 0) return { r: 0, g: 0, b: 0 };
  return { r: r / total, g: g / total, b: b / total };
};

/**
 * Compute the average color with more weight given to pixes at the center.
 *
 * @param {!ImageData} imageData Image data to analyze.
 * @return {{r: number, g: number, b: number}} weighted average color.
 */
filter.getWeightedAverageColor = function(imageData) {
  var data = imageData.data;
  var width = imageData.width;
  var height = imageData.height;

  var total = 0;
  var r = 0;
  var g = 0;
  var b = 0;

  var center = Math.floor(width / 2);
  var maxDist = center * Math.sqrt(2);
  maxDist *= 2; // Weaken the effect of distance

  var i = 0;
  for (var x = 0; x != width; x++) {
    for (var y = 0; y != height; y++) {
      var dist = Math.sqrt(
          (x - center) * (x - center) + (y - center) * (y - center));
      var weight = (maxDist - dist) / maxDist;

      total += weight;
      r += data[i++] * weight;
      g += data[i++] * weight;
      b += data[i++] * weight;
      i++;
    }
  }
  if (total == 0) return { r: 0, g: 0, b: 0 };
  return { r: r / total, g: g / total, b: b / total };
};

/**
 * Copy part of src image to dst, applying matrix color filter on-the-fly.
 *
 * The copied part of src should completely fit into dst (there is no clipping
 * on either side).
 *
 * @param {!Array<number>} matrix 3x3 color matrix.
 * @param {!ImageData} dst Destination image data.
 * @param {!ImageData} src Source image data.
 * @param {number} offsetX X offset in source to start processing.
 * @param {number} offsetY Y offset in source to start processing.
 */
filter.colorMatrix3x3 = function(matrix, dst, src, offsetX, offsetY) {
  var c11 = filter.floatToFixedPoint(matrix[0]);
  var c12 = filter.floatToFixedPoint(matrix[1]);
  var c13 = filter.floatToFixedPoint(matrix[2]);
  var c21 = filter.floatToFixedPoint(matrix[3]);
  var c22 = filter.floatToFixedPoint(matrix[4]);
  var c23 = filter.floatToFixedPoint(matrix[5]);
  var c31 = filter.floatToFixedPoint(matrix[6]);
  var c32 = filter.floatToFixedPoint(matrix[7]);
  var c33 = filter.floatToFixedPoint(matrix[8]);

  var dstData = dst.data;
  var dstWidth = dst.width;
  var dstHeight = dst.height;

  var srcData = src.data;
  var srcWidth = src.width;
  var srcHeight = src.height;

  if (offsetX < 0 || offsetX + dstWidth > srcWidth ||
      offsetY < 0 || offsetY + dstHeight > srcHeight)
    throw new Error('Invalid offset');

  // Javascript is not very good at inlining constants.
  // We inline manually and assert that the constant is equal to the variable.
  if (filter.FIXED_POINT_SHIFT != 16)
    throw new Error('Wrong fixed point shift');

  var dstIndex = 0;
  for (var y = 0; y != dstHeight; y++) {
    var srcIndex = (offsetX + (offsetY + y) * srcWidth) * 4;
    for (var x = 0; x != dstWidth; x++) {
      var r = srcData[srcIndex++];
      var g = srcData[srcIndex++];
      var b = srcData[srcIndex++];
      srcIndex++;

      var rNew = r * c11 + g * c12 + b * c13;
      var gNew = r * c21 + g * c22 + b * c23;
      var bNew = r * c31 + g * c32 + b * c33;

      if (rNew < 0) {
        dstData[dstIndex++] = 0;
      } else if (rNew > 0xFF0000) {
        dstData[dstIndex++] = 0xFF;
      } else {
        dstData[dstIndex++] = rNew >> 16;
      }

      if (gNew < 0) {
        dstData[dstIndex++] = 0;
      } else if (gNew > 0xFF0000) {
        dstData[dstIndex++] = 0xFF;
      } else {
        dstData[dstIndex++] = gNew >> 16;
      }

      if (bNew < 0) {
        dstData[dstIndex++] = 0;
      } else if (bNew > 0xFF0000) {
        dstData[dstIndex++] = 0xFF;
      } else {
        dstData[dstIndex++] = bNew >> 16;
      }

      dstIndex++;
    }
  }
};

/**
 * Return a convolution filter function bound to specific weights.
 *
 * @param {!Array<number>} weights Weights for the convolution matrix
 *     (not normalized).
 * @return {function(!ImageData,!ImageData,number,number)} Convolution filter.
 */
filter.createConvolutionFilter = function(weights) {
  // Normalize the weights to sum to 1.
  var total = 0;
  for (var i = 0; i != weights.length; i++) {
    total += weights[i] * (i ? 4 : 1);
  }

  var normalized = [];
  for (i = 0; i != weights.length; i++) {
    normalized.push(weights[i] / total);
  }
  for (; i < 4; i++) {
    normalized.push(0);
  }

  var maxWeightedSum = 0xFF *
      Math.abs(normalized[0]) +
      Math.abs(normalized[1]) * 4 +
      Math.abs(normalized[2]) * 4 +
      Math.abs(normalized[3]) * 4;
  if (maxWeightedSum > filter.MAX_FLOAT_VALUE)
    throw new Error('convolve5x5 cannot convert the weights to fixed point');

  return filter.convolve5x5.bind(null, normalized);
};

/**
 * Creates matrix filter.
 * @param {!Array<number>} matrix Color transformation matrix.
 * @return {function(!ImageData,!ImageData,number,number)} Matrix filter.
 */
filter.createColorMatrixFilter = function(matrix) {
  for (var r = 0; r != 3; r++) {
    var maxRowSum = 0;
    for (var c = 0; c != 3; c++) {
      maxRowSum += 0xFF * Math.abs(matrix[r * 3 + c]);
    }
    if (maxRowSum > filter.MAX_FLOAT_VALUE)
      throw new Error(
          'colorMatrix3x3 cannot convert the matrix to fixed point');
  }
  return filter.colorMatrix3x3.bind(null, matrix);
};

/**
 * Return a blur filter.
 * @param {{radius: number, strength: number}} options Blur options.
 * @return {function(!ImageData,!ImageData,number,number)} Blur filter.
 */
filter.blur = function(options) {
  if (options.radius == 1)
    return filter.createConvolutionFilter(
        [1, options.strength]);
  else if (options.radius == 2)
    return filter.createConvolutionFilter(
        [1, options.strength, options.strength]);
  else
    return filter.createConvolutionFilter(
        [1, options.strength, options.strength, options.strength]);
};

/**
 * Return a sharpen filter.
 * @param {{radius: number, strength: number}} options Sharpen options.
 * @return {function(!ImageData,!ImageData,number,number)} Sharpen filter.
 */
filter.sharpen = function(options) {
  if (options.radius == 1)
    return filter.createConvolutionFilter(
        [5, -options.strength]);
  else if (options.radius == 2)
    return filter.createConvolutionFilter(
        [10, -options.strength, -options.strength]);
  else
    return filter.createConvolutionFilter(
        [15, -options.strength, -options.strength, -options.strength]);
};

/**
 * Return an exposure filter.
 * @param {{brightness: number, contrast: number}} options exposure options.
 * @return {function(!ImageData,!ImageData,number,number)} Exposure filter.
 */
filter.exposure = function(options) {
  var pixelMap = filter.precompute(
      255,
      function(value) {
        if (options.brightness > 0) {
          value *= (1 + options.brightness);
        } else {
          value += (0xFF - value) * options.brightness;
        }
        return 0x80 +
            (value - 0x80) * Math.tan((options.contrast + 1) * Math.PI / 4);
      });

  return filter.mapPixels.bind(null, pixelMap, pixelMap, pixelMap);
};

/**
 * Return a color autofix filter.
 * @param {{histogram:
 *     {r: !Array<number>, g: !Array<number>, b: !Array<number>}}} options
 *     Histogram for autofix.
 * @return {function(!ImageData,!ImageData,number,number)} Autofix filter.
 */
filter.autofix = function(options) {
  return filter.mapPixels.bind(null,
      filter.autofix.stretchColors(options.histogram.r),
      filter.autofix.stretchColors(options.histogram.g),
      filter.autofix.stretchColors(options.histogram.b));
};

/**
 * Return a conversion table that stretches the range of colors used
 * in the image to 0..255.
 * @param {!Array<number>} channelHistogram Histogram to calculate range.
 * @return {!Uint8Array} Color mapping array.
 */
filter.autofix.stretchColors = function(channelHistogram) {
  var range = filter.autofix.getRange(channelHistogram);
  return filter.precompute(
      255,
      function(x) {
        return (x - range.first) / (range.last - range.first) * 255;
      }
  );
};

/**
 * Return a range that encloses non-zero elements values in a histogram array.
 * @param {!Array<number>} channelHistogram Histogram to analyze.
 * @return {{first: number, last: number}} Channel range in histogram.
 */
filter.autofix.getRange = function(channelHistogram) {
  var first = 0;
  while (first < channelHistogram.length && channelHistogram[first] == 0)
    first++;

  var last = channelHistogram.length - 1;
  while (last >= 0 && channelHistogram[last] == 0)
    last--;

  if (first >= last) // Stretching does not make sense
    return {first: 0, last: channelHistogram.length - 1};
  else
    return {first: first, last: last};
};

/**
 * Minimum channel offset that makes visual difference. If autofix calculated
 * offset is less than SENSITIVITY, probably autofix is not needed.
 * Reasonable empirical value.
 * @type {number}
 * @const
 */
filter.autofix.SENSITIVITY = 8;

/**
 * @param {!Array<number>} channelHistogram Histogram to analyze.
 * @return {boolean} True if stretching this range to 0..255 would make
 *                   a visible difference.
 */
filter.autofix.needsStretching = function(channelHistogram) {
  var range = filter.autofix.getRange(channelHistogram);
  return (range.first >= filter.autofix.SENSITIVITY ||
          range.last <= 255 - filter.autofix.SENSITIVITY);
};

/**
 * @param {{r: !Array<number>, g: !Array<number>, b: !Array<number>}}
 *     histogram
 * @return {boolean} True if the autofix would make a visible difference.
 */
filter.autofix.isApplicable = function(histogram) {
  return filter.autofix.needsStretching(histogram.r) ||
         filter.autofix.needsStretching(histogram.g) ||
         filter.autofix.needsStretching(histogram.b);
};
