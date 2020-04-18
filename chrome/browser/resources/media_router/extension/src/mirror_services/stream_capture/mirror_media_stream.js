// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Media stream for use with tab and desktop capture.
 *
 * A media stream has a video and/or audio track.  Each capture source (i.e.,
 * tab) should use its own MediaStream object and call start() to initiate
 * capture.
 */

goog.provide('mr.mirror.MirrorMediaStream');

goog.require('mr.Assertions');
goog.require('mr.Logger');
goog.require('mr.MirrorAnalytics');
goog.require('mr.PlatformUtils');
goog.require('mr.mirror.Config');
goog.require('mr.mirror.Error');

/**
 * Constructs a new MediaStream that will capture media according to
 *     captureParams.
 */
mr.mirror.MirrorMediaStream = class {
  /**
   * @param {!mr.mirror.CaptureParameters} captureParams
   */
  constructor(captureParams) {
    /** @private {!mr.mirror.CaptureParameters} */
    this.captureParams_ = captureParams;

    /** @private {?function()} */
    this.onStreamEnded_ = null;

    /** @private {?MediaStream} */
    this.mediaStream_ = null;

    /** @private {mr.Logger} */
    this.logger_ = mr.Logger.getInstance('mr.mirror.MirrorMediaStream');
  }

  /**
   * @param {?function()} onStreamEnded Invoked when the stream fires an
   *  onended event.
   */
  setOnStreamEnded(onStreamEnded) {
    this.onStreamEnded_ = onStreamEnded;
  }

  /**
   * @return {!mr.mirror.CaptureParameters}
   */
  getCaptureParams() {
    return this.captureParams_;
  }

  /**
   * @return {?MediaStream}
   */
  getMediaStream() {
    return this.mediaStream_;
  }

  /**
   * Starts capturing media and sets audioTrack and videoTrack.
   * @return {!Promise<!mr.mirror.MirrorMediaStream>} Fulfilled when capture
   *     has started.
   */
  start() {
    if (this.captureParams_.isTab()) {
      return this.startTabCapturing_();
    } else if (this.captureParams_.isOffscreenTab()) {
      return this.startOffscreenTabCapturing_();
    } else {
      return this.startDesktopCapturing_();
    }
  }

  /**
   * @return {!Promise<!mr.mirror.MirrorMediaStream>} Fulfilled when capture
   *     has started.
   * @private
   */
  startTabCapturing_() {
    const constraints = this.captureParams_.toMediaConstraints();
    this.logger_.info(
        'Starting tab capture with constraints ' + JSON.stringify(constraints));

    return new Promise((resolve, reject) => {
      // Note: There's a subtle reason to NOT pass |resolve| as the
      // callback function to the chrome.tabCapture.capture() call here.
      // Because this is an extension API, when an error occurs, the value
      // in chrome.runtime.lastError is only available during the call to
      // the callback function.  However, the Promise.then() method runs
      // its function at a later time, when chrome.runtime.lastError is no
      // longer set.
      chrome.tabCapture.capture(constraints, stream => {
        if (stream) {
          this.setStream_(stream);
          resolve(this);
        } else {
          reject(this.createTabCaptureError_());
        }
      });

      // Set a timer to reject the promise after a delay.
      //

      window.setTimeout(() => {
        // In normal usage, this will be a no-op because this promise will
        // already have been resolved by the call to
        // chrome.tabCapture.capture above.
        reject(new mr.mirror.Error(
            'chrome.tabCapture.capture failed to call its callback',
            mr.MirrorAnalytics.CapturingFailure.CAPTURE_TAB_TIMEOUT));
      }, mr.mirror.MirrorMediaStream.TAB_CAPTURE_TIMEOUT_);
    });
  }

  /**
   * @param {?Event} event The ended event.
   * @private
   */
  handleTrackEnded_(event) {
    if (event) {
      this.logger_.info(
          () => 'Track ' + JSON.stringify(event.target) + ' ended');
    }
    this.stop();
  }

  /**
   * Returns an Error corresponding to a chrome.tabCapture error.
   * @return {!mr.mirror.Error} The corresponding error.
   * @private
   */
  createTabCaptureError_() {
    // As of Chrome 51, when |stream| is null, chrome.runtime.lastError.message
    // should always be set to a non-empty string.  If it is not, fall back to
    // the default error message so everyone can yell at miu@.
    if (chrome.runtime.lastError && chrome.runtime.lastError.message) {
      return new mr.mirror.Error(
          chrome.runtime.lastError.message,
          mr.MirrorAnalytics.CapturingFailure.TAB_FAIL);
    } else {
      return new mr.mirror.Error(
          mr.mirror.MirrorMediaStream.EMPTY_STREAM_,
          mr.MirrorAnalytics.CapturingFailure.CAPTURE_TAB_FAIL_EMPTY_STREAM);
    }
  }

  /**
   * Requests a screen capture source from the user via a native dialog and
   * returns the source ID, or rejects if a timeout is reached or the user
   * cancels.
   * @param {number=} timeoutMillis The timeout in milliseconds.
   * @return {!Promise<string>} Fulfilled with the source ID.
   * @private
   */
  requestScreenCaptureSourceId_(
      timeoutMillis = mr.mirror.MirrorMediaStream.WINDOW_PICKER_TIMEOUT_) {
    return new Promise((resolve, reject) => {
      const desktopChooserConfig = ['screen', 'audio'];
      if (mr.PlatformUtils.getCurrentOS() == mr.PlatformUtils.OS.LINUX) {
        desktopChooserConfig.push('window');
      }
      let requestId;
      // Wait 60 seconds and then cancel the picker and reject the
      // promise.

      const timeoutId = window.setTimeout(() => {
        if (requestId) {
          chrome.desktopCapture.cancelChooseDesktopMedia(requestId);
        }
        reject(new mr.mirror.Error(
            'timeout',
            mr.MirrorAnalytics.CapturingFailure
                .CAPTURE_DESKTOP_FAIL_ERROR_TIMEOUT));
      }, timeoutMillis);
      // https://developer.chrome.com/extensions/desktopCapture#method-chooseDesktopMedia
      requestId = chrome.desktopCapture.chooseDesktopMedia(
          desktopChooserConfig, sourceId => {
            window.clearTimeout(timeoutId);
            if (!sourceId) {
              // User cancelled the desktop media selector prompt.
              reject(new mr.mirror.Error(
                  'User cancelled capture dialog',
                  mr.MirrorAnalytics.CapturingFailure
                      .CAPTURE_DESKTOP_FAIL_ERROR_USER_CANCEL));
            } else {
              resolve(sourceId);
            }
          });
    });
  }

  /**
   * Generates a screen capture MediaStream from the given
   * MediaStreamConstraints.
   * @param {!MediaStreamConstraints} constraints The constraints object to use.
   * @return {!Promise<MediaStream>} Fulfilled with the MediaStream from
   *     capture.
   * @private
   */
  generateScreenCaptureStream_(constraints) {
    return new Promise((resolve, reject) => {
      this.logger_.info(
          () => 'Starting desktop capture with constraints ' +
              JSON.stringify(constraints));
      navigator.mediaDevices.getUserMedia(constraints)
          .then(
              stream => {
                if (!stream) {
                  // NOTE(miu): This implies that getUserMedia is broken, and it
                  // may also be breaking chrome.tabCapture.
                  reject(new mr.mirror.Error(
                      mr.mirror.MirrorMediaStream.EMPTY_STREAM_,
                      mr.MirrorAnalytics.CapturingFailure.DESKTOP_FAIL));
                }
                this.setStream_(stream);
                resolve(stream);
              },
              error => {
                let errorReason =
                    mr.MirrorAnalytics.CapturingFailure.DESKTOP_FAIL;
                // Certain errors indicate the user cancelled the request.
                // https://www.w3.org/TR/mediacapture-streams/#methods-5
                if (error.name == 'NotAllowedError') {
                  errorReason = mr.MirrorAnalytics.CapturingFailure
                                    .CAPTURE_DESKTOP_FAIL_ERROR_USER_CANCEL;
                }
                reject(new mr.mirror.Error(
                    `${error.name} ${error.constraintName}: ${error.message}`,
                    errorReason));
              });
    });
  }

  /**
   * @return {!Promise<!mr.mirror.MirrorMediaStream>} Fulfilled when capture
   *     has started.
   * @private
   */
  startDesktopCapturing_() {
    if (mr.mirror.Config.isDesktopAudioCaptureAvailable &&
        this.captureParams_.mirrorSettings.shouldCaptureAudio &&
        !this.captureParams_.mirrorSettings.shouldCaptureVideo) {
      return this
          .generateScreenCaptureStream_(
              this.captureParams_.toMediaConstraints())
          .then(_ => this);
    }

    // Video capture requires asking the user to pick which screen to capture.

    return this.requestScreenCaptureSourceId_().then(sourceId => {
      const constraints = this.captureParams_.toMediaConstraints(sourceId);
      return this.generateScreenCaptureStream_(constraints).then(_ => this);
    });
  }

  /**
   * @return {!Promise<!mr.mirror.MirrorMediaStream>} Fulfilled when capture
   *     has started.
   * @private
   */
  startOffscreenTabCapturing_() {
    mr.Assertions.assert(!!this.captureParams_.offscreenTabUrl);
    const constraints = this.captureParams_.toMediaConstraints();
    this.logger_.info(
        () => 'Starting offscreen tab capture with constraints ' +
            JSON.stringify(constraints));
    return new Promise((resolve, reject) => {
      chrome.tabCapture.captureOffscreenTab(
          this.captureParams_.offscreenTabUrl.toString(), constraints,
          stream => {
            if (stream) {
              this.setStream_(stream);
              resolve(this);
            } else {
              reject(this.createTabCaptureError_());
            }
          });
    });
  }

  /**
   * @param {!MediaStream} stream
   * @private
   */
  setStream_(stream) {
    this.mediaStream_ = stream;
    mr.Assertions.assert(
        stream.getAudioTracks().length || stream.getVideoTracks().length,
        'Expecting at least one audio or video track.');
    // For desktop capturing, users may stop capturing via desktop capturing's
    // own stop button, which triggers onended event.
    stream.getTracks().forEach(track => {
      track.onended = this.handleTrackEnded_.bind(this);
    });
  }

  /**
   * Stops captured streams.
   */
  stop() {
    if (!this.mediaStream_) return;
    this.mediaStream_.getTracks().forEach(track => {
      track.onended = null;
      track.stop();
    });
    this.mediaStream_ = null;
    if (this.onStreamEnded_) {
      this.onStreamEnded_();
    }
  }
};


/**
 * The number of milliseconds to wait after for the browser to call the callback
 * function after calling chrome.tabCapture.capture.
 * @private @const {number}
 */
mr.mirror.MirrorMediaStream.TAB_CAPTURE_TIMEOUT_ = 5000;


/**
 * @private @const {number}
 */
mr.mirror.MirrorMediaStream.WINDOW_PICKER_TIMEOUT_ = 60000;


/**
 * Error messaging for reporting of empty streams.
 * @private @const {string}
 */
mr.mirror.MirrorMediaStream.EMPTY_STREAM_ = 'empty_stream';
