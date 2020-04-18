// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Loads a thumbnail using provided url. In CANVAS mode, loaded images
 * are attached as <canvas> element, while in IMAGE mode as <img>.
 * <canvas> renders faster than <img>, however has bigger memory overhead.
 *
 * @param {Entry} entry File entry.
 * @param {ThumbnailLoader.LoaderType=} opt_loaderType Canvas or Image loader,
 *     default: IMAGE.
 * @param {Object=} opt_metadata Metadata object.
 * @param {string=} opt_mediaType Media type.
 * @param {Array<ThumbnailLoader.LoadTarget>=} opt_loadTargets The list of load
 *     targets in preferential order. The default value is [CONTENT_METADATA,
 *     EXTERNAL_METADATA, FILE_ENTRY].
 * @param {number=} opt_priority Priority, the highest is 0. default: 2.
 * @constructor
 */
function ThumbnailLoader(entry, opt_loaderType, opt_metadata, opt_mediaType,
    opt_loadTargets, opt_priority) {
  var loadTargets = opt_loadTargets || [
    ThumbnailLoader.LoadTarget.CONTENT_METADATA,
    ThumbnailLoader.LoadTarget.EXTERNAL_METADATA,
    ThumbnailLoader.LoadTarget.FILE_ENTRY
  ];

  /**
   * @private {Entry}
   * @const
   */
  this.entry_ = entry;

  this.mediaType_ = opt_mediaType || FileType.getMediaType(entry);
  this.loaderType_ = opt_loaderType || ThumbnailLoader.LoaderType.IMAGE;
  this.metadata_ = opt_metadata;
  this.priority_ = (opt_priority !== undefined) ? opt_priority : 2;
  this.transform_ = null;

  /**
   * @type {?ThumbnailLoader.LoadTarget}
   * @private
   */
  this.loadTarget_ = null;

  if (!opt_metadata) {
    this.thumbnailUrl_ = entry.toURL();  // Use the URL directly.
    this.loadTarget_ = ThumbnailLoader.LoadTarget.FILE_ENTRY;
    return;
  }

  this.fallbackUrl_ = null;
  this.thumbnailUrl_ = null;
  if (opt_metadata.external && opt_metadata.external.customIconUrl)
    this.fallbackUrl_ = opt_metadata.external.customIconUrl;
  var mimeType = opt_metadata && opt_metadata.contentMimeType;

  for (var i = 0; i < loadTargets.length; i++) {
    switch (loadTargets[i]) {
      case ThumbnailLoader.LoadTarget.CONTENT_METADATA:
        if (opt_metadata.thumbnail && opt_metadata.thumbnail.url) {
          this.thumbnailUrl_ = opt_metadata.thumbnail.url;
          this.transform_ =
              opt_metadata.thumbnail && opt_metadata.thumbnail.transform;
          this.loadTarget_ = ThumbnailLoader.LoadTarget.CONTENT_METADATA;
        }
        break;
      case ThumbnailLoader.LoadTarget.EXTERNAL_METADATA:
        if (opt_metadata.external && opt_metadata.external.thumbnailUrl &&
            (!opt_metadata.external.present ||
             !FileType.isImage(entry, mimeType))) {
          this.thumbnailUrl_ = opt_metadata.external.thumbnailUrl;
          this.croppedThumbnailUrl_ = opt_metadata.external.croppedThumbnailUrl;
          this.loadTarget_ = ThumbnailLoader.LoadTarget.EXTERNAL_METADATA;
        }
        break;
      case ThumbnailLoader.LoadTarget.FILE_ENTRY:
        if (FileType.isImage(entry, mimeType) ||
            FileType.isVideo(entry, mimeType) ||
            FileType.isRaw(entry, mimeType)) {
          this.thumbnailUrl_ = entry.toURL();
          this.transform_ =
              opt_metadata.media && opt_metadata.media.imageTransform;
          this.loadTarget_ = ThumbnailLoader.LoadTarget.FILE_ENTRY;
        }
        break;
      default:
        assertNotReached('Unkonwn load type: ' + loadTargets[i]);
    }
    if (this.thumbnailUrl_)
      break;
  }

  if (!this.thumbnailUrl_ && this.fallbackUrl_) {
    // Use fallback as the primary thumbnail.
    this.thumbnailUrl_ = this.fallbackUrl_;
    this.fallbackUrl_ = null;
  } // else the generic thumbnail based on the media type will be used.
}

/**
 * In percents (0.0 - 1.0), how much area can be cropped to fill an image
 * in a container, when loading a thumbnail in FillMode.AUTO mode.
 * The default 30% value allows to fill 16:9, 3:2 pictures in 4:3 element.
 * @const {number}
 */
ThumbnailLoader.AUTO_FILL_THRESHOLD_DEFAULT_VALUE = 0.3;

/**
 * Type of displaying a thumbnail within a box.
 * @enum {number}
 */
ThumbnailLoader.FillMode = {
  FILL: 0,  // Fill whole box. Image may be cropped.
  FIT: 1,   // Keep aspect ratio, do not crop.
  OVER_FILL: 2,  // Fill whole box with possible stretching.
  AUTO: 3   // Try to fill, but if incompatible aspect ratio, then fit.
};

/**
 * Optimization mode for downloading thumbnails.
 * @enum {number}
 */
ThumbnailLoader.OptimizationMode = {
  NEVER_DISCARD: 0,    // Never discards downloading. No optimization.
  DISCARD_DETACHED: 1  // Canceled if the container is not attached anymore.
};

/**
 * Type of element to store the image.
 * @enum {number}
 */
ThumbnailLoader.LoaderType = {
  IMAGE: 0,
  CANVAS: 1
};

/**
 * Load target of ThumbnailLoader.
 * @enum {string}
 */
ThumbnailLoader.LoadTarget = {
  // e.g. Drive thumbnail, FSP thumbnail.
  EXTERNAL_METADATA: 'externalMetadata',
  // e.g. EXIF thumbnail.
  CONTENT_METADATA: 'contentMetadata',
  // Image file itself.
  FILE_ENTRY: 'fileEntry'
};

/**
 * Maximum thumbnail's width when generating from the full resolution image.
 * @const
 * @type {number}
 */
ThumbnailLoader.THUMBNAIL_MAX_WIDTH = 500;

/**
 * Maximum thumbnail's height when generating from the full resolution image.
 * @const
 * @type {number}
 */
ThumbnailLoader.THUMBNAIL_MAX_HEIGHT = 500;

/**
 * Returns the target of loading.
 * @return {?ThumbnailLoader.LoadTarget}
 */
ThumbnailLoader.prototype.getLoadTarget = function() {
  return this.loadTarget_;
};

/**
 * Loads and attaches an image.
 *
 * @param {Element} box Container element.
 * @param {ThumbnailLoader.FillMode} fillMode Fill mode.
 * @param {ThumbnailLoader.OptimizationMode=} opt_optimizationMode Optimization
 *     for downloading thumbnails. By default optimizations are disabled.
 * @param {function(Image)=} opt_onSuccess Success callback, accepts the image.
 * @param {function()=} opt_onError Error callback.
 * @param {function()=} opt_onGeneric Callback for generic image used.
 * @param {number=} opt_autoFillThreshold Auto fill threshold.
 * @param {number=} opt_boxWidth Container box's width. If not specified, the
 *     given |box|'s clientWidth will be used.
 * @param {number=} opt_boxHeight Container box's height. If not specified, the
 *     given |box|'s clientHeight will be used.
 */
ThumbnailLoader.prototype.load = function(box, fillMode, opt_optimizationMode,
    opt_onSuccess, opt_onError, opt_onGeneric, opt_autoFillThreshold,
    opt_boxWidth, opt_boxHeight) {
  opt_optimizationMode = opt_optimizationMode ||
      ThumbnailLoader.OptimizationMode.NEVER_DISCARD;

  if (!this.thumbnailUrl_) {
    // Relevant CSS rules are in file_types.css.
    box.setAttribute('generic-thumbnail', this.mediaType_);
    if (opt_onGeneric) opt_onGeneric();
    return;
  }

  this.cancel();
  this.canvasUpToDate_ = false;
  this.image_ = new Image();
  this.image_.setAttribute('alt', this.entry_.name);
  this.image_.onload = function() {
    this.attachImage(assert(box), fillMode, opt_autoFillThreshold,
                     opt_boxWidth, opt_boxHeight);
    if (opt_onSuccess)
      opt_onSuccess(this.image_);
  }.bind(this);
  this.image_.onerror = function() {
    if (opt_onError)
      opt_onError();
    if (this.fallbackUrl_) {
      this.thumbnailUrl_ = this.fallbackUrl_;
      this.fallbackUrl_ = null;
      this.load(box, fillMode, opt_optimizationMode, opt_onSuccess);
    } else {
      box.setAttribute('generic-thumbnail', this.mediaType_);
    }
  }.bind(this);

  if (this.image_.src) {
    console.warn('Thumbnail already loaded: ' + this.thumbnailUrl_);
    return;
  }

  // TODO(mtomasz): Smarter calculation of the requested size.
  var wasAttached = box.ownerDocument.contains(box);
  var modificationTime = this.metadata_ &&
                         this.metadata_.filesystem &&
                         this.metadata_.filesystem.modificationTime &&
                         this.metadata_.filesystem.modificationTime.getTime();
  this.taskId_ = ImageLoaderClient.loadToImage(
      this.thumbnailUrl_,
      this.image_,
      {
        maxWidth: ThumbnailLoader.THUMBNAIL_MAX_WIDTH,
        maxHeight: ThumbnailLoader.THUMBNAIL_MAX_HEIGHT,
        cache: true,
        priority: this.priority_,
        timestamp: modificationTime,
        orientation: this.transform_
      },
      function() {},
      function() {
        this.image_.onerror(new Event('load-error'));
      }.bind(this),
      function() {
        if (opt_optimizationMode ==
            ThumbnailLoader.OptimizationMode.DISCARD_DETACHED &&
            !box.ownerDocument.contains(box)) {
          // If the container is not attached, then invalidate the download.
          return false;
        }
        return true;
      });
};

/**
 * Loads thumbnail as data url. If data url of thumbnail can be fetched from
 * metadata, this fetches it from it. Otherwise, this tries to load it from
 * thumbnail loader.
 * Compared with ThumbnailLoader.load, this method does not provide a
 * functionality to fit image to a box.
 *
 * @param {ThumbnailLoader.FillMode} fillMode Only FIT and OVER_FILL is
 *     supported. This takes effect only when external thumbnail source is used.
 * @return {!Promise<{data:string, width:number, height:number}>} A promise
 *     which is resolved when data url is fetched.
 *
 * TODO(yawano): Support cancel operation.
 */
ThumbnailLoader.prototype.loadAsDataUrl = function(fillMode) {
  assert(fillMode === ThumbnailLoader.FillMode.FIT ||
      fillMode === ThumbnailLoader.FillMode.OVER_FILL);

  return new Promise(function(resolve, reject) {
    // Load by using ImageLoaderClient.
    var modificationTime = this.metadata_ &&
                           this.metadata_.filesystem &&
                           this.metadata_.filesystem.modificationTime &&
                           this.metadata_.filesystem.modificationTime.getTime();
    var thumbnailUrl = this.thumbnailUrl_;
    var options = {
      maxWidth: ThumbnailLoader.THUMBNAIL_MAX_WIDTH,
      maxHeight: ThumbnailLoader.THUMBNAIL_MAX_HEIGHT,
      cache: true,
      priority: this.priority_,
      timestamp: modificationTime,
      orientation: this.transform_
    };

    if (fillMode === ThumbnailLoader.FillMode.OVER_FILL) {
      // Use cropped thumbnail url if available.
      thumbnailUrl = this.croppedThumbnailUrl_ ?
          this.croppedThumbnailUrl_ : this.thumbnailUrl_;

      // Set crop option to image loader. Since image of croppedThumbnailUrl_ is
      // 360x360 with current implemenation, it's no problem to crop it.
      options['width'] = 360;
      options['height'] = 360;
      options['crop'] = true;
    }

    ImageLoaderClient.getInstance().load(
        thumbnailUrl,
        function(result) {
          if (result.status === 'success')
            resolve(result);
          else
            reject(result);
        },
        options);
  }.bind(this));
};

/**
 * Cancels loading the current image.
 */
ThumbnailLoader.prototype.cancel = function() {
  if (this.taskId_) {
    this.image_.onload = function() {};
    this.image_.onerror = function() {};
    ImageLoaderClient.getInstance().cancel(this.taskId_);
    this.taskId_ = null;
  }
};

/**
 * @return {boolean} True if a valid image is loaded.
 */
ThumbnailLoader.prototype.hasValidImage = function() {
  return !!(this.image_ && this.image_.width && this.image_.height);
};

/**
 * @return {number} Image width.
 */
ThumbnailLoader.prototype.getWidth = function() {
  return this.image_.width;
};

/**
 * @return {number} Image height.
 */
ThumbnailLoader.prototype.getHeight = function() {
  return this.image_.height;
};

/**
 * Load an image but do not attach it.
 *
 * @param {function(boolean)} callback Callback, parameter is true if the image
 *     has loaded successfully or a stock icon has been used.
 */
ThumbnailLoader.prototype.loadDetachedImage = function(callback) {
  if (!this.thumbnailUrl_) {
    callback(true);
    return;
  }

  this.cancel();
  this.canvasUpToDate_ = false;
  this.image_ = new Image();
  this.image_.onload = callback.bind(null, true);
  this.image_.onerror = callback.bind(null, false);

  // TODO(mtomasz): Smarter calculation of the requested size.
  var modificationTime = this.metadata_ &&
                         this.metadata_.filesystem &&
                         this.metadata_.filesystem.modificationTime &&
                         this.metadata_.filesystem.modificationTime.getTime();
  this.taskId_ = ImageLoaderClient.loadToImage(
      this.thumbnailUrl_,
      this.image_,
      {
        maxWidth: ThumbnailLoader.THUMBNAIL_MAX_WIDTH,
        maxHeight: ThumbnailLoader.THUMBNAIL_MAX_HEIGHT,
        cache: true,
        priority: this.priority_,
        timestamp: modificationTime,
        orientation: this.transform_
      },
      function() {},
      function() {
        this.image_.onerror(new Event('load-error'));
      }.bind(this));
};

/**
 * Renders the thumbnail into either canvas or an image element.
 * @private
 */
ThumbnailLoader.prototype.renderMedia_ = function() {
  if (this.loaderType_ !== ThumbnailLoader.LoaderType.CANVAS)
    return;

  if (!this.canvas_)
    this.canvas_ = document.createElement('canvas');

  // Copy the image to a canvas if the canvas is outdated.
  // At this point, image transformation is not applied because we attach style
  // attribute to an img element in attachImage() instead.
  if (!this.canvasUpToDate_) {
    this.canvas_.width = this.image_.width;
    this.canvas_.height = this.image_.height;
    var context = this.canvas_.getContext('2d');
    context.drawImage(this.image_, 0, 0);
    this.canvasUpToDate_ = true;
  }
};

/**
 * Attach the image to a given element.
 * @param {!Element} box Container element.
 * @param {ThumbnailLoader.FillMode} fillMode Fill mode.
 * @param {number=} opt_autoFillThreshold Threshold value which is used for fill
 *     mode auto.
 * @param {number=} opt_boxWidth Container box's width. If not specified, the
 *     given |box|'s clientWidth will be used.
 * @param {number=} opt_boxHeight Container box's height. If not specified, the
 *     given |box|'s clientHeight will be used.
 */
ThumbnailLoader.prototype.attachImage = function(
    box, fillMode, opt_autoFillThreshold, opt_boxWidth, opt_boxHeight) {
  if (!this.hasValidImage()) {
    box.setAttribute('generic-thumbnail', this.mediaType_);
    return;
  }

  this.renderMedia_();
  var attachableMedia = this.loaderType_ === ThumbnailLoader.LoaderType.CANVAS ?
      this.canvas_ : this.image_;

  var autoFillThreshold = opt_autoFillThreshold ||
      ThumbnailLoader.AUTO_FILL_THRESHOLD_DEFAULT_VALUE;
  ThumbnailLoader.centerImage_(box, attachableMedia, fillMode,
      autoFillThreshold, opt_boxWidth, opt_boxHeight);

  if (attachableMedia.parentNode !== box) {
    box.textContent = '';
    box.appendChild(attachableMedia);
  }

  if (!this.taskId_)
    attachableMedia.classList.add('cached');
};

/**
 * Gets the loaded image.
 *
 * @return {Image|HTMLCanvasElement} Either image or a canvas object.
 */
ThumbnailLoader.prototype.getImage = function() {
  this.renderMedia_();
  return (this.loaderType_ === ThumbnailLoader.LoaderType.IMAGE) ?
      this.image_ : this.canvas_;
};

/**
 * Update the image style to fit/fill the container.
 *
 * Using webkit center packing does not align the image properly, so we need
 * to wait until the image loads and its dimensions are known, then manually
 * position it at the center.
 *
 * @param {Element} box Containing element.
 * @param {Image|HTMLCanvasElement} img Element containing an image.
 * @param {ThumbnailLoader.FillMode} fillMode Fill mode.
 * @param {number} autoFillThreshold Threshold value which is used for fill mode
 *     auto.
 * @param {number=} opt_boxWidth Container box's width. If not specified, the
 *     given |box|'s clientWidth will be used.
 * @param {number=} opt_boxHeight Container box's height. If not specified, the
 *     given |box|'s clientHeight will be used.
 * @private
 */
ThumbnailLoader.centerImage_ = function(
    box, img, fillMode, autoFillThreshold, opt_boxWidth,
    opt_boxHeight) {
  var imageWidth = img.width;
  var imageHeight = img.height;

  var fractionX;
  var fractionY;

  var boxWidth = opt_boxWidth || box.clientWidth;
  var boxHeight = opt_boxHeight || box.clientHeight;

  var fill;
  switch (fillMode) {
    case ThumbnailLoader.FillMode.FILL:
    case ThumbnailLoader.FillMode.OVER_FILL:
      fill = true;
      break;
    case ThumbnailLoader.FillMode.FIT:
      fill = false;
      break;
    case ThumbnailLoader.FillMode.AUTO:
      var imageRatio = imageWidth / imageHeight;
      var boxRatio = 1.0;
      if (boxWidth && boxHeight)
        boxRatio = boxWidth / boxHeight;
      // Cropped area in percents.
      var ratioFactor = boxRatio / imageRatio;
      fill = (ratioFactor >= 1.0 - autoFillThreshold) &&
             (ratioFactor <= 1.0 + autoFillThreshold);
      break;
  }

  if (boxWidth && boxHeight) {
    // When we know the box size we can position the image correctly even
    // in a non-square box.
    var fitScaleX = boxWidth / imageWidth;
    var fitScaleY = boxHeight / imageHeight;

    var scale = fill ?
        Math.max(fitScaleX, fitScaleY) :
        Math.min(fitScaleX, fitScaleY);

    if (fillMode !== ThumbnailLoader.FillMode.OVER_FILL)
      scale = Math.min(scale, 1);  // Never overscale.

    fractionX = imageWidth * scale / boxWidth;
    fractionY = imageHeight * scale / boxHeight;
  } else {
    // We do not know the box size so we assume it is square.
    // Compute the image position based only on the image dimensions.
    // First try vertical fit or horizontal fill.
    fractionX = imageWidth / imageHeight;
    fractionY = 1;
    if ((fractionX < 1) === !!fill) {  // Vertical fill or horizontal fit.
      fractionY = 1 / fractionX;
      fractionX = 1;
    }
  }

  function percent(fraction) {
    return (fraction * 100).toFixed(2) + '%';
  }

  img.style.width = percent(fractionX);
  img.style.height = percent(fractionY);
  img.style.left = percent((1 - fractionX) / 2);
  img.style.top = percent((1 - fractionY) / 2);
};
