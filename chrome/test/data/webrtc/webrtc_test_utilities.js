// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains duplicated functions in of
// content/test/data/media/webrtc_test_utilities.js
// TODO(phoglund): Eliminate this copy and rewrite the
// WebRtcBrowserTest.TestVgaReturnsTwoSimulcastStreams test to use the browser
// tests style instead.

// These must match with how the video and canvas tags are declared in html.
const VIDEO_TAG_WIDTH = 320;
const VIDEO_TAG_HEIGHT = 240;

// Number of test events to occur before the test pass. When the test pass,
// the function gAllEventsOccured is called.
var gNumberOfExpectedEvents = 0;

// Number of events that currently have occurred.
var gNumberOfEvents = 0;

var gAllEventsOccured = function () {};

// Use this function to set a function that will be called once all expected
// events has occurred.
function setAllEventsOccuredHandler(handler) {
  gAllEventsOccured = handler;
}

// See comments on waitForVideo.
function detectVideoIn(videoElementName, callback) {
  var width = VIDEO_TAG_WIDTH;
  var height = VIDEO_TAG_HEIGHT;
  var videoElement = $(videoElementName);
  var canvas = $(videoElementName + '-canvas');
  var waitVideo = setInterval(function() {
    var context = canvas.getContext('2d');
    context.drawImage(videoElement, 0, 0, width, height);
    var pixels = context.getImageData(0, 0, width, height).data;

    if (isVideoPlaying(pixels, width, height)) {
      clearInterval(waitVideo);
      callback();
    }
  }, 100);
}

/**
 * Blocks test success until the provided videoElement has playing video.
 *
 * @param videoElementName The id of the video element. There must also be a
 *     canvas somewhere in the DOM tree with the id |videoElementName|-canvas.
 * @param callback The callback to call.
 */
function waitForVideo(videoElement) {
  document.title = 'Waiting for video...';
  addExpectedEvent();
  detectVideoIn(videoElement, function () { eventOccured(); });
}

/**
 * Blocks test success until the provided peerconnection reports the signaling
 * state 'stable'.
 *
 * @param peerConnection The peer connection to look at.
 */
function waitForConnectionToStabilize(peerConnection) {
  addExpectedEvent();
  var waitForStabilization = setInterval(function() {
    if (peerConnection.signalingState == 'stable') {
      clearInterval(waitForStabilization);
      eventOccured();
    }
  }, 100);
}

/**
 * Adds an expectation for an event to occur at some later point. You may call
 * this several times per test, which will each add an expected event. Once all
 * events have occurred, we'll call the "all events occurred" handler which will
 * generally succeed the test or move the test to the next phase.
 */
function addExpectedEvent() {
  ++gNumberOfExpectedEvents;
}

// See comment on addExpectedEvent.
function eventOccured() {
  ++gNumberOfEvents;
  if (gNumberOfEvents == gNumberOfExpectedEvents) {
    gAllEventsOccured();
  }
}

// This very basic video verification algorithm will be satisfied if any
// pixels are nonzero in a small sample area in the middle. It relies on the
// assumption that a video element with null source just presents zeroes.
function isVideoPlaying(pixels, width, height) {
  // Sample somewhere near the middle of the image.
  var middle = width * height / 2;
  for (var i = 0; i < 20; i++) {
    if (pixels[middle + i] > 0) {
      return true;
    }
  }
  return false;
}
