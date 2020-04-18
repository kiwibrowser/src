// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * The base class for simple filters that only modify the image content
 * but do not modify the image dimensions.
 * @param {string} name
 * @param {string} title
 * @constructor
 * @struct
 * @extends {ImageEditorMode}
 */
ImageEditorMode.Adjust = function(name, title) {
  ImageEditorMode.call(this, name, title);

  /**
   * @type {boolean}
   * @const
   */
  this.implicitCommit = true;

  /**
   * @type {?string}
   * @private
   */
  this.doneMessage_ = null;

  /**
   * @type {number}
   * @private
   */
  this.viewportGeneration_ = 0;

  /**
   * @type {?function(!ImageData,!ImageData,number,number)}
   * @private
   */
  this.filter_ = null;

  /**
   * @type {HTMLCanvasElement}
   * @private
   */
  this.canvas_ = null;

  /**
   * @private {ImageData}
   */
  this.previewImageData_ = null;

  /**
   * @private {ImageData}
   */
  this.originalImageData_ = null;
};

ImageEditorMode.Adjust.prototype = {
  __proto__: ImageEditorMode.prototype
};

/**
 * Gets command to do filter.
 *
 * @return {Command.Filter} Filter command.
 */
ImageEditorMode.Adjust.prototype.getCommand = function() {
  if (!this.filter_)
    return null;

  return new Command.Filter(this.name, this.filter_, this.doneMessage_);
};

/** @override */
ImageEditorMode.Adjust.prototype.cleanUpUI = function() {
  ImageEditorMode.prototype.cleanUpUI.apply(this, arguments);
  this.hidePreview();
};

/**
 * Hides preview.
 */
ImageEditorMode.Adjust.prototype.hidePreview = function() {
  if (this.canvas_) {
    this.canvas_.parentNode.removeChild(this.canvas_);
    this.canvas_ = null;
  }
};

/** @override */
ImageEditorMode.Adjust.prototype.cleanUpCaches = function() {
  this.filter_ = null;
  this.previewImageData_ = null;
};

/** @override */
ImageEditorMode.Adjust.prototype.reset = function() {
  ImageEditorMode.prototype.reset.call(this);
  this.hidePreview();
  this.cleanUpCaches();
};

/** @override */
ImageEditorMode.Adjust.prototype.update = function(options) {
  ImageEditorMode.prototype.update.apply(this, arguments);
  this.updatePreviewImage_(options);
};

/**
 * Copy the source image data for the preview.
 * Use the cached copy if the viewport has not changed.
 * @param {Object} options Options that describe the filter. It it is null, it
 *     does not update current filter.
 * @private
 */
ImageEditorMode.Adjust.prototype.updatePreviewImage_ = function(options) {
  assert(this.getViewport());

  var isPreviewImageInvalidated = false;

  // Update filter.
  if (options) {
    // We assume filter names are used in the UI directly.
    // This will have to change with i18n.
    this.filter_ = this.createFilter(options);
    isPreviewImageInvalidated = true;
  }

  // Update canvas size and/or transformation.
  if (!this.previewImageData_ ||
      this.viewportGeneration_ !== this.getViewport().getCacheGeneration()) {
    this.viewportGeneration_ = this.getViewport().getCacheGeneration();

    if (!this.canvas_)
      this.canvas_ = this.getImageView().createOverlayCanvas();

    this.getImageView().setupDeviceBuffer(this.canvas_);
    var canvas = this.getImageView().getImageCanvasWith(
        this.canvas_.width, this.canvas_.height);
    var context = canvas.getContext('2d');
    this.originalImageData_ = context.getImageData(0, 0,
        this.canvas_.width, this.canvas_.height);
    this.previewImageData_ = context.getImageData(0, 0,
        this.canvas_.width, this.canvas_.height);

    isPreviewImageInvalidated = true;
  } else {
    this.getImageView().setTransform_(
        assert(this.canvas_), assert(this.getViewport()));
  }

  // Update preview image with applying filter.
  if (isPreviewImageInvalidated) {
    assert(this.originalImageData_);
    assert(this.previewImageData_);

    ImageUtil.trace.resetTimer('preview');
    this.filter_(this.previewImageData_, this.originalImageData_, 0, 0);
    ImageUtil.trace.reportTimer('preview');

    this.canvas_.getContext('2d').putImageData(this.previewImageData_, 0, 0);
  }
};

/** @override */
ImageEditorMode.Adjust.prototype.draw = function() {
  this.updatePreviewImage_(null);
};

/*
 * Own methods
 */

/**
 * Creates a filter.
 * @param {!Object} options A map of filter-specific options.
 * @return {function(!ImageData,!ImageData,number,number)} Created function.
 */
ImageEditorMode.Adjust.prototype.createFilter = function(options) {
  return filter.create(this.name, options);
};

/**
 * A base class for color filters that are scale independent.
 * @constructor
 * @param {string} name The mode name.
 * @param {string} title The mode title.
 * @extends {ImageEditorMode.Adjust}
 * @struct
 */
ImageEditorMode.ColorFilter = function(name, title) {
  ImageEditorMode.Adjust.call(this, name, title);
};

ImageEditorMode.ColorFilter.prototype = {
  __proto__: ImageEditorMode.Adjust.prototype
};

/**
 * Gets a histogram from a thumbnail.
 * @return {{r: !Array<number>, g: !Array<number>, b: !Array<number>}}
 *    histogram.
 */
ImageEditorMode.ColorFilter.prototype.getHistogram = function() {
  return filter.getHistogram(this.getImageView().getThumbnail());
};

/**
 * Exposure/contrast filter.
 * @constructor
 * @extends {ImageEditorMode.ColorFilter}
 * @struct
 */
ImageEditorMode.Exposure = function() {
  ImageEditorMode.ColorFilter.call(this, 'exposure', 'GALLERY_EXPOSURE');
};

ImageEditorMode.Exposure.prototype = {
  __proto__: ImageEditorMode.ColorFilter.prototype
};

/** @override */
ImageEditorMode.Exposure.prototype.createTools = function(toolbar) {
  toolbar.addRange('brightness', 'GALLERY_BRIGHTNESS', -1, 0, 1, 100);
  toolbar.addRange('contrast', 'GALLERY_CONTRAST', -1, 0, 1, 100);
};

/**
 * Autofix.
 * @constructor
 * @struct
 * @extends {ImageEditorMode.ColorFilter}
 */
ImageEditorMode.Autofix = function() {
  ImageEditorMode.ColorFilter.call(this, 'autofix', 'GALLERY_AUTOFIX');
  this.doneMessage_ = 'GALLERY_FIXED';
};

ImageEditorMode.Autofix.prototype = {
  __proto__: ImageEditorMode.ColorFilter.prototype
};

/** @override */
ImageEditorMode.Autofix.prototype.isApplicable = function() {
  return this.getImageView().hasValidImage() &&
      filter.autofix.isApplicable(this.getHistogram());
};

/**
 * Applies autofix.
 */
ImageEditorMode.Autofix.prototype.apply = function() {
  this.update({histogram: this.getHistogram()});
};

/**
 * Instant Autofix.
 * @constructor
 * @extends {ImageEditorMode.Autofix}
 * @struct
 */
ImageEditorMode.InstantAutofix = function() {
  ImageEditorMode.Autofix.call(this);
  this.instant = true;
};

ImageEditorMode.InstantAutofix.prototype = {
  __proto__: ImageEditorMode.Autofix.prototype
};

/** @override */
ImageEditorMode.InstantAutofix.prototype.setUp = function() {
  ImageEditorMode.Autofix.prototype.setUp.apply(this, arguments);
  this.apply();
};
