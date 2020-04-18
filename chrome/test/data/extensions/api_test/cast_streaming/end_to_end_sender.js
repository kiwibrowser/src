// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This code uses the tab capture and Cast streaming APIs to capture the content
// and send it to a Cast receiver end-point controlled by
// CastStreamingApiTest code.  It generates audio/video test patterns that
// rotate cyclicly, and these test patterns are checked for by an in-process
// Cast receiver to confirm the correct end-to-end functionality of the Cast
// streaming API.
//
// Once everything is set up and fully operational, chrome.test.succeed() is
// invoked as a signal for the end-to-end testing to proceed.  If any step in
// the setup process fails, chrome.test.fail() is invoked.

// The test pattern cycles as a color fill of red, then green, then blue; paired
// with successively higher-frequency tones.
var colors = [ [ 255, 0, 0 ], [ 0, 255, 0 ], [ 0, 0, 255 ] ];
var freqs = [ 200, 500, 1800 ];
var curTestIdx = 0;

function updateTestPattern() {
  if (!this.canvas) {
    this.canvas = document.createElement("canvas");
    this.canvas.width = 320;
    this.canvas.height = 200;
    this.canvas.style.position = "absolute";
    this.canvas.style.top = "0px";
    this.canvas.style.left = "0px";
    this.canvas.style.width = "100%";
    this.canvas.style.height = "100%";
    document.body.appendChild(this.canvas);
  }
  var context = this.canvas.getContext("2d");
  // Fill with solid color.
  context.fillStyle = "rgb(" + colors[curTestIdx] + ")";
  context.fillRect(0, 0, this.canvas.width, this.canvas.height);
  // Draw the circle that moves around the page.
  context.fillStyle = "rgb(" + colors[(curTestIdx + 1) % colors.length] + ")";
  context.beginPath();
  if (!this.frameNumber) {
    this.frameNumber = 1;
  } else {
    ++this.frameNumber;
  }
  var i = this.frameNumber % 200;
  var t = (this.frameNumber + 3000) * (0.01 + i / 8000.0);
  var x = (Math.sin(t) * 0.45 + 0.5) * this.canvas.width;
  var y = (Math.cos(t * 0.9) * 0.45 + 0.5) * this.canvas.height;
  context.arc(x, y, 16, 0, 2 * Math.PI, false);
  context.closePath();
  context.fill();

  if (!this.audioContext) {
    this.audioContext = new AudioContext();
    this.gainNode = this.audioContext.createGain();
    this.gainNode.gain.value = 0.5;
    this.gainNode.connect(this.audioContext.destination);
  }
  if (!this.oscillator ||
      this.oscillator.frequency.value != freqs[curTestIdx]) {
    // Note: We recreate the oscillator each time because this switches the
    // audio frequency immediately.  Re-using the same oscillator tends to take
    // several hundred milliseconds to ramp-up/down the frequency.
    if (this.oscillator) {
      this.oscillator.stop();
      this.oscillator.disconnect();
    }
    this.oscillator = this.audioContext.createOscillator();
    this.oscillator.type = OscillatorNode.SINE;
    this.oscillator.frequency.value = freqs[curTestIdx];
    this.oscillator.connect(this.gainNode);
    this.oscillator.start();
  }
}

// Called to render each frame of video, and also to update the main fill color
// and audio frequency.
function renderTestPatternLoop() {
  requestAnimationFrame(renderTestPatternLoop);
  updateTestPattern();

  if (!this.stepTimeMillis) {
    this.stepTimeMillis = 100;
  }
  var now = new Date().getTime();
  if (!this.nextSteppingAt) {
    this.nextSteppingAt = now + this.stepTimeMillis;
  } else if (now >= this.nextSteppingAt) {
    ++curTestIdx;
    if (curTestIdx >= colors.length) {  // Completed a cycle.
      curTestIdx = 0;
      // Increase the wait time between switching test patterns for overloaded
      // bots that aren't capturing all the frames of video.
      this.stepTimeMillis *= 1.25;
    }
    this.nextSteppingAt = now + this.stepTimeMillis;
  }
}

chrome.test.runTests([
  function sendTestPatterns() {
    // The receive port changes between browser_test invocations, and is passed
    // as an query parameter in the URL.
    var recvPort;
    var aesKey;
    var aesIvMask;
    try {
      recvPort = parseInt(window.location.search.match(/(\?|&)port=(\d+)/)[2]);
      chrome.test.assertTrue(recvPort > 0);
      aesKey = window.location.search.match(/(\?|&)aesKey=(\w+)/)[2];
      chrome.test.assertTrue(aesKey.length > 0);
      aesIvMask = window.location.search.match(/(\?|&)aesIvMask=(\w+)/)[2];
      chrome.test.assertTrue(aesIvMask.length > 0);
    } catch (err) {
      chrome.test.fail("Error parsing query params -- " + err.message);
      return;
    }

    // Set to true if you want to confirm the sender color/tone changes are
    // working, without starting tab capture and Cast sending.
    if (false) {
      renderTestPatternLoop();
      return;
    }

    var width = 320;
    var height = 200;
    var frameRate = 15;

    chrome.tabCapture.capture(
        { video: true,
          audio: true,
          videoConstraints: {
            mandatory: {
              minWidth: width,
              minHeight: height,
              maxWidth: width,
              maxHeight: height,
              maxFrameRate: frameRate,
            }
          }
        },
        function startStreamingTestPatterns(captureStream) {
          chrome.test.assertTrue(!!captureStream);
          chrome.cast.streaming.session.create(
              captureStream.getAudioTracks()[0],
              captureStream.getVideoTracks()[0],
              function (audioId, videoId, udpId) {
                chrome.cast.streaming.udpTransport.setDestination(
                    udpId, { address: "127.0.0.1", port: recvPort } );
                var rtpStream = chrome.cast.streaming.rtpStream;
                var audioParams = rtpStream.getSupportedParams(audioId)[0];
                audioParams.payload.aesKey = aesKey;
                audioParams.payload.aesIvMask = aesIvMask;
                rtpStream.start(audioId, audioParams);
                var videoParams = rtpStream.getSupportedParams(videoId)[0];
                videoParams.payload.maxFrameRate = frameRate;
                videoParams.payload.aesKey = aesKey;
                videoParams.payload.aesIvMask = aesIvMask;
                rtpStream.start(videoId, videoParams);
                renderTestPatternLoop();
                chrome.test.succeed();
              });
        });
  }
]);
