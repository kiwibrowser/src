/*
 * Copyright 2017 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/*jshint esversion: 6 */

/**
 * A loopback peer connection with one or more streams.
 */
class PeerConnection {
  /**
   * Creates a loopback peer connection. One stream per supplied resolution is
   * created.
   * @param {!Element} videoElement the video element to render the feed on.
   * @param {!Array<!{x: number, y: number}>} resolutions. A width of -1 will
   *     result in disabled video for that stream.
   * @param {?boolean=} cpuOveruseDetection Whether to enable
   *     googCpuOveruseDetection (lower video quality if CPU usage is high).
   *     Default is null which means that the constraint is not set at all.
   */
  constructor(videoElement, resolutions, cpuOveruseDetection=null) {
    this.localConnection = null;
    this.remoteConnection = null;
    this.remoteView = videoElement;
    this.streams = [];
    // Ensure sorted in descending order to conveniently request the highest
    // resolution first through GUM later.
    this.resolutions = resolutions.slice().sort((x, y) => y.w - x.w);
    this.activeStreamIndex = resolutions.length - 1;
    this.badResolutionsSeen = 0;
    if (cpuOveruseDetection !== null) {
      this.pcConstraints = {
        'optional': [{'googCpuOveruseDetection': cpuOveruseDetection}]
      };
    }
  }

  /**
   * Starts the connections. Triggers GetUserMedia and starts
   * to render the video on {@code this.videoElement}.
   * @return {!Promise} a Promise that resolves when everything is initalized.
   */
  start() {
    // getUserMedia fails if we first request a low resolution and
    // later a higher one. Hence, sort resolutions above and
    // start with the highest resolution here.
    const promises = this.resolutions.map((resolution) => {
      const constraints = createMediaConstraints(resolution);
      return navigator.mediaDevices
        .getUserMedia(constraints)
        .then((stream) => this.streams.push(stream));
    });
    return Promise.all(promises).then(() => {
      // Start with the smallest video to not overload the machine instantly.
      return this.onGetUserMediaSuccess_(this.streams[this.activeStreamIndex]);
    })
  };

  /**
   * Verifies that the state of the streams are good. The state is good if all
   * streams are active and their video elements report the resolution the
   * stream is in. Video elements are allowed to report bad resolutions
   * numSequentialBadResolutionsForFailure times before failure is reported
   * since video elements occasionally report bad resolutions during the tests
   * when we manipulate the streams frequently.
   * @param {number=} numSequentialBadResolutionsForFailure number of bad
   *     resolution observations in a row before failure is reported.
   * @param {number=} allowedDelta allowed difference between expected and
   *     actual resolution. We have seen videos assigned a resolution one pixel
   *     off from the requested.
   */
  verifyState(numSequentialBadResolutionsForFailure=10, allowedDelta=1) {
    this.verifyAllStreamsActive_();
    const expectedResolution = this.resolutions[this.activeStreamIndex];
    if (expectedResolution.w < 0 || expectedResolution.h < 0) {
      // Video is disabled.
      return;
    }
    if (!isWithin(
            this.remoteView.videoWidth, expectedResolution.w, allowedDelta) ||
        !isWithin(
            this.remoteView.videoHeight, expectedResolution.h, allowedDelta)) {
      this.badResolutionsSeen++;
    } else if (
        this.badResolutionsSeen < numSequentialBadResolutionsForFailure) {
      // Reset the count, but only if we have not yet reached the limit. If the
      // limit is reached, let keep the error state.
      this.badResolutionsSeen = 0;
    }
    if (this.badResolutionsSeen >= numSequentialBadResolutionsForFailure) {
      failTest(
          'Expected video resolution ' +
          resStr(expectedResolution.w, expectedResolution.h) +
          ' but got another resolution ' + this.badResolutionsSeen +
          ' consecutive times. Last resolution was: ' +
          resStr(this.remoteView.videoWidth, this.remoteView.videoHeight));
    }
  }

  verifyAllStreamsActive_() {
    if (this.streams.some((x) => !x.active)) {
      failTest('At least one media stream is not active')
    }
  }

  /**
   * Switches to a random stream, i.e., use a random resolution of the
   * resolutions provided to the constructor.
   * @return {!Promise} A promise that resolved when everything is initialized.
   */
  switchToRandomStream() {
    const localStreams = this.localConnection.getLocalStreams();
    const track = localStreams[0];
    if (track != null) {
      this.localConnection.removeStream(track);
      const newStreamIndex = Math.floor(Math.random() * this.streams.length);
      return this.addStream_(this.streams[newStreamIndex])
          .then(() => this.activeStreamIndex = newStreamIndex);
    } else {
      return Promise.resolve();
    }
  }

  onGetUserMediaSuccess_(stream) {
    this.localConnection = new RTCPeerConnection(null, this.pcConstraints);
    this.localConnection.onicecandidate = (event) => {
      this.onIceCandidate_(this.remoteConnection, event);
    };
    this.remoteConnection = new RTCPeerConnection(null, this.pcConstraints);
    this.remoteConnection.onicecandidate = (event) => {
      this.onIceCandidate_(this.localConnection, event);
    };
    this.remoteConnection.onaddstream = (e) => {
      this.remoteView.srcObject = e.stream;
    };
    return this.addStream_(stream);
  }

  addStream_(stream) {
    this.localConnection.addStream(stream);
    return this.localConnection
        .createOffer({offerToReceiveAudio: 1, offerToReceiveVideo: 1})
        .then((desc) => this.onCreateOfferSuccess_(desc), logError);
  }

  onCreateOfferSuccess_(desc) {
    this.localConnection.setLocalDescription(desc);
    this.remoteConnection.setRemoteDescription(desc);
    return this.remoteConnection.createAnswer().then(
        (desc) => this.onCreateAnswerSuccess_(desc), logError);
  };

  onCreateAnswerSuccess_(desc) {
    this.remoteConnection.setLocalDescription(desc);
    this.localConnection.setRemoteDescription(desc);
  };

  onIceCandidate_(connection, event) {
    if (event.candidate) {
      connection.addIceCandidate(new RTCIceCandidate(event.candidate));
    }
  };
}

/**
 * Checks if a value is within an expected value plus/minus a delta.
 * @param {number} actual
 * @param {number} expected
 * @param {number} delta
 * @return {boolean}
 */
function isWithin(actual, expected, delta) {
  return actual <= expected + delta && actual >= actual - delta;
}

/**
 * Creates constraints for use with GetUserMedia.
 * @param {!{x: number, y: number}} widthAndHeight Video resolution.
 */
function createMediaConstraints(widthAndHeight) {
  let constraint;
  if (widthAndHeight.w < 0) {
    constraint = false;
  } else {
    constraint = {
      width: {exact: widthAndHeight.w},
      height: {exact: widthAndHeight.h}
    };
  }
  return {
    audio: true,
    video: constraint
  };
}

function resStr(width, height) {
  return `${width}x${height}`
}

function logError(err) {
  console.error(err);
}
