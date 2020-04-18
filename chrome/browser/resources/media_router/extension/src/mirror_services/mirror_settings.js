// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Tab-mirroring settings, including video bitrate, video
 *               resolution, and audio bitrate.
 *

 */

goog.provide('mr.mirror.Settings');
goog.provide('mr.mirror.VideoCodec');

goog.require('mr.Logger');
goog.require('mr.PlatformUtils');


/**
 * @enum {string}
 */
mr.mirror.VideoCodec = {
  VP8: 'VP8',
  // This is the internal codename for hardware-encoded H264. for V1 mirroring.
  CAST1: 'CAST1',
  H264: 'H264',

  // This is a fake codec for retransmissions used in WebRTC.
  RTX: 'rtx'
};



/**
 * Settings that affect capture and transport (via Cast Streaming or WebRTC).
 * Generally, these should all be left unchanged from their defaults.
 * Overriding them is only meant for development or user experiments.
 *
 * WARNING: If making any changes to defaults here, it is on *you* to confirm
 * that all downstream consumers of these settings will behave correctly with
 * the new values. If not, see usage notes below for per-sink adjustments.
 *
 * Usage: Generally, this class should be instantiated by mr.Provider
 * implementations only. The provider creates a new instance, and then must
 * adjust any properties based on both the sender's and sink's capabilities.
 * It should also, as a final step, freeze the settings object to prevent any
 * downstream code from making changes. Example provider code:
 *
 *   getMirrorSettings(sinkId) {
 *     // Constrain default settings to a sink that is only capable of standard
 *     // definition (lower resolution and no high frame rate support).
 *     const settings = new mr.mirror.Settings();
 *     settings.maxWidth = Math.min(settings.maxWidth, 640);
 *     settings.maxHeight = Math.min(settings.maxWidth, 360);
 *     settings.maxFrameRate = Math.min(settings.maxFrameRate, 30);
 *
 *     // Override: Some sinks might require the sender to do the letterboxing.
 *     settings.senderSideLetterboxing =
 *         !this.canSinkHandleLetterboxing_(sinkId);
 *
 *     settings.makeFinalAdjustmentsAndFreeze();
 *     return settings;
 *   }
 *
 */
mr.mirror.Settings = class {
  constructor() {
    /**
     * Maximum video width in pixels.
     *
     * @export {number}
     */
    this.maxWidth = 1920;

    /**
     * Maximum video height in pixels.
     *
     * @export {number}
     */
    this.maxHeight = 1080;

    /**
     * Minimum video width in pixels.
     *
     * @export {number}
     */
    this.minWidth = 180;

    /**
     * Minimum video height in pixels.
     *
     * @export {number}
     */
    this.minHeight = 180;

    /**
     * Whether the screen capture must handle letterboxing/pillarboxing. If
     * false (more desired), the receiver will handle it. When setting this to
     * true, please see comments for getMinDimensionsToMatchAspectRatio().
     *
     * @export {boolean}
     */
    this.senderSideLetterboxing = false;

    /**
     * The minimum frame rate for captures. Well-behaved clients can handle a
     * minimum frame rate of zero, which prevents wasting system resources
     * sender-side. Unfortunately, not all clients are well-behaved...
     *
     * @export {number}
     */
    this.minFrameRate = 0;

    /**
     * The maximum frame rate for captures.
     *
     * @export {number}
     */
    this.maxFrameRate = 30;

    /**
     * Minimum video bitrate in kbps.
     *
     * @export {number}
     */
    this.minVideoBitrate = 300;

    /**
     * Maximum video bitrate in kbps.
     *
     * @export {number}
     */
    this.maxVideoBitrate = 5000;

    /**
     * Target audio bitrate in kbps (0 means automatic).
     *
     * @export {number}
     */
    this.audioBitrate = 0;

    /**
     * Maximum end-to-end latency (in milliseconds).
     *
     * @export {number}
     */
    this.maxLatencyMillis = 800;

    /**
     * Minimum end-to-end latency (in milliseconds). This allows cast streaming
     * to adaptively lower latency in interactive streaming scenarios.
     * This setting currently applies to cast streaming only.
     *

     *
     * @export {number}
     */
    this.minLatencyMillis = 400;

    /**
     * Starting end-to-end latency for animated content (in milliseconds).
     *
     * @export {number}
     */
    this.animatedLatencyMillis = 400;

    /**
     * Enable DSCP?
     * This setting currently applies to cast streaming only.
     *
     * @export {boolean}
     */
    this.dscpEnabled =
        [
          mr.PlatformUtils.OS.MAC, mr.PlatformUtils.OS.LINUX,
          mr.PlatformUtils.OS.CHROMEOS
        ].includes(mr.PlatformUtils.getCurrentOS()) ||
        mr.PlatformUtils.isWindows8OrNewer();

    /**
     * Whether to enable network transport logging.
     *
     * @export {boolean}
     */
    this.enableLogging = true;

    /**
     * Whether an attempt should be made to use TDLS.
     * This setting currently applies to cast streaming only.
     *
     * @export {boolean}
     */
    this.useTdls = false;


    /**
     * Whether video should be captured. This could be influenced by the
     * application and/or the sink's capabilities.
     * @export {boolean}
     */
    this.shouldCaptureVideo = true;

    /**
     * Whether audio should be captured. This could be influenced by the
     * application and/or the sink's capabilities.
     * @export {boolean}
     */
    this.shouldCaptureAudio = true;

    // For development, debugging, or integration testing use only!
    const overrides = window.localStorage ?
        window.localStorage.getItem(mr.mirror.Settings.OverridesKey) :
        null;
    if (overrides) {
      try {
        const parsedOverrides = JSON.parse(String(overrides));
        if (parsedOverrides instanceof Object) {
          this.update_(/** @type {!Object} */ (parsedOverrides));
          mr.Logger.getInstance('mr.mirror.Settings')
              .warning(
                  () => 'Initial mr.mirror.Settings overridden to: ' +
                      this.toJsonString());
        } else {
          throw Error(
              `localStorage[${mr.mirror.Settings.OverridesKey}] ` +
              `does not parse as an Object: ${overrides}`);
        }
      } catch (exception) {
        mr.Logger.getInstance('mr.mirror.Settings')
            .error(
                mr.mirror.Settings.OverridesKey + ' must be of the form ' +
                    '\'{"maxWidth":640, "maxHeight":360}\'.',
                exception);
        // Prevent mirroring from starting if overrides are present and not
        // parseable.
        throw new Error('Overrides not parseable.  See ERROR log for details.');
      }
    }
  }

  /**
   * @return {!mr.mirror.Settings}
   */
  clone() {
    const settings = new mr.mirror.Settings();
    settings.update_(this);
    return settings;
  }

  /**
   * Returns the properties of this Settings object as a JSON-formatted string.
   * @return {!string}
   */
  toJsonString() {
    return JSON.stringify(this, (key, value) => {
      // Only public fields are included in the stringified output.
      if (key.length == 0 || !key.endsWith('_')) {
        return value;
      }
      return undefined;
    });
  }

  /**
   * Update this object to have the same settings as another object.
   * @param {!Object} settings The properties to apply, which may be any subset
   *     of all the public fields of this class.
   * @private
   */
  update_(settings) {
    // Override Closure-compiler "access on a struct" error.
    const self = /** @type {!Object} */ (this);
    for (const key of Object.keys(settings)) {
      if (key.endsWith('_') || (typeof settings[key] !== typeof self[key])) {
        continue;
      }
      self[key] = settings[key];
    }
  }

  /**
   * Make mandatory system-wide bounds adjustments and then freeze this Settings
   * object. See example usage in class-level comments.
   */
  makeFinalAdjustmentsAndFreeze() {
    this.clampMaxDimensionsToScreenSize_();
    Object.freeze(this);
  }

  /**
   * Adjusts the maxWidth/maxHeight to within the size of the user's screen, and
   * rounds down to a standard 16:9 resolution (i.e., width is 0 modulo 160 and
   * height is 0 modulo 90).  This prevents performance problems due to:
   *   1. The pre-capture fullscreen size being something way larger than the
   *       system was designed for (e.g., 1080p on a Daisy Chromebook).
   *   2. The receiver dealing with scaling from an oddball resolution to a
   *       standard resolution (e.g., 1366x768 --> 1280x720).
   * @private
   */
  clampMaxDimensionsToScreenSize_() {
    const widthStep = 160;
    const heightStep = 90;
    const screenWidth = mr.mirror.Settings.getScreenWidth();
    const screenHeight = mr.mirror.Settings.getScreenHeight();
    const x = this.maxWidth * screenHeight;
    const y = this.maxHeight * screenWidth;
    let clampedWidth = 0;
    let clampedHeight = 0;
    if (y < x) {
      clampedWidth = Math.min(this.maxWidth, screenWidth);
      clampedWidth = clampedWidth - (clampedWidth % widthStep);
      clampedHeight = clampedWidth * heightStep / widthStep;
    } else {
      clampedHeight = Math.min(this.maxHeight, screenHeight);
      clampedHeight = clampedHeight - (clampedHeight % heightStep);
      clampedWidth = clampedHeight * widthStep / heightStep;
    }
    if (clampedWidth < Math.max(widthStep, this.minWidth) ||
        clampedHeight < Math.max(heightStep, this.minHeight)) {
      clampedWidth = Math.max(widthStep, this.minWidth);
      clampedHeight = Math.max(heightStep, this.minHeight);
    }

    this.maxWidth = clampedWidth;
    this.maxHeight = clampedHeight;
  }

  /**
   * Returns alternate |minWidth| and |minHeight| values that match the aspect
   * ratio of |maxWidth| and |maxHeight| to the nearest integer. Some of the
   * capture APIs will then interpret the matching aspect ratios to mean that
   * the sender should letterbox/pillarbox the video, rather than allowing the
   * receiver to handle it. This method does NOT modify any properties of this
   * Settings object.
   * @return {!{width: number, height: number}} New minimum width/height values,
   *     as described.
   */
  getMinDimensionsToMatchAspectRatio() {
    if (this.minAndMaxAspectRatiosAreSimilar()) {
      return {width: this.minWidth, height: this.minHeight};
    }

    let a = this.maxWidth;
    let b = this.maxHeight;
    while (b != 0) {
      const remainder = a % b;
      a = b;
      b = remainder;
    }
    // Note: |a| now contains the greatest common divisor.
    let width = this.maxWidth / a;
    let height = this.maxHeight / a;
    if (width < this.minWidth || height < this.minHeight) {
      // Increase to respect the current min width/Height setting.
      const upFactor = Math.max(this.minWidth / width, this.minHeight / height);
      width *= upFactor;
      height *= upFactor;
      // ...and make sure the increase does not now exceed the max width/height.
      if (width > this.maxWidth || height > this.maxHeight) {
        width = this.maxWidth;
        height = this.maxHeight;
      }
    }
    width = Math.round(width);
    height = Math.round(height);
    return {width, height};
  }

  /**
   * @return {boolean} Returns true if the aspect ratios of the min and max
   *     dimensions are within one part in one hundred of each other.
   */
  minAndMaxAspectRatiosAreSimilar() {
    if (this.minHeight == 0 || this.maxHeight == 0) {
      return false;
    }
    // Source: content/renderer/media/media_stream_video_capturer_source.cc (in
    // Chromium project, circa 2016).
    const ratioOfMinSize = Math.floor(100.0 * this.minWidth / this.minHeight);
    const ratioOfMaxSize = Math.floor(100.0 * this.maxWidth / this.maxHeight);
    return ratioOfMinSize == ratioOfMaxSize;
  }

  /** @return {number} */
  static getScreenWidth() {
    return screen.width;
  }

  /** @return {number} */
  static getScreenHeight() {
    return screen.height;
  }
};


/**
 * The key for retrieving settings overrides from localStorage.
 * @type {string}
 */
mr.mirror.Settings.OverridesKey = 'mr.mirror.Settings.Overrides';
