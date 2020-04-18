// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains a test harness for testing the behavior of pages running
// inside off-screen tabs:
//
//   1. It provides a simple way to formulate a data URI containing a full HTML
//      document that executes arbitrary script.  The arbitrary script running
//      in the off-screen tab calls setFillColor() to expose its current state
//      by rendering a color fill.  In addition, the off-screen tab continuously
//      paints changes that force the tab capture implementation to continuously
//      capture new video frames.
//
//   2. In the extension's JavaScript context, a waitAnForExpectedColor()
//      function is provided to sample the video frames of the captured
//      off-screen tab until the center pixel contains one of the colors in a
//      set of expected colors.

// Width/Height of off-screen tab, rendered content, and captured content.
var width = 160;
var height = 120;

// Capture frame rate.
var frameRate = 8;

// Return a full HTML document that executes the |script|.  The script calls
// setFillColor() to expose changes to its internal state.
function makeOffscreenTabTestDocument(script) {
  return '\
<html><body style="background-color:black; margin:0; padding:0;"></body>\n\
<script>\n\
var redColor = [255, 0, 0];\n\
var greenColor = [0, 255, 0];\n\
var blueColor = [0, 0, 255];\n\
\n\
var fillColor = [0, 0, 0];\n\
function setFillColor(color) {\n\
  fillColor = color;\n\
}\n\
\n\
var debugMessage = "";\n\
function setDebugMessage(msg) {\n\
  debugMessage = msg;\n\
}\n\
\n\
function updateTestPattern() {\n\
  if (!this.canvas) {\n\
    this.canvas = document.createElement("canvas");\n\
    this.canvas.width = ' + width + ';\n\
    this.canvas.height = ' + height + ';\n\
    this.canvas.style.position = "absolute";\n\
    this.canvas.style.top = "0px";\n\
    this.canvas.style.left = "0px";\n\
    this.canvas.style.width = "100%";\n\
    this.canvas.style.height = "100%";\n\
    document.body.appendChild(this.canvas);\n\
  }\n\
  var context = this.canvas.getContext("2d");\n\
  // Fill with solid color.\n\
  context.fillStyle = "rgb(" + fillColor + ")";\n\
  context.fillRect(0, 0, this.canvas.width, this.canvas.height);\n\
  // Draw the circle that moves around the page.\n\
  var inverseColor =\n\
      [255 - fillColor[0], 255 - fillColor[1], 255 - fillColor[2]];\n\
  if (debugMessage) {\n\
    context.fillStyle = "rgb(" + inverseColor + ")";\n\
    context.font = "xx-small monospace";\n\
    context.fillText(debugMessage, 0, 0);\n\
  }\n\
  context.fillStyle = "rgba(" + inverseColor + ", 0.5)";\n\
  context.beginPath();\n\
  if (!this.frameNumber) {\n\
    this.frameNumber = 1;\n\
  } else {\n\
    ++this.frameNumber;\n\
  }\n\
  var i = this.frameNumber % 200;\n\
  var t = (this.frameNumber + 3000) * (0.01 + i / 8000.0);\n\
  var x = (Math.sin(t) * 0.45 + 0.5) * this.canvas.width;\n\
  var y = (Math.cos(t * 0.9) * 0.45 + 0.5) * this.canvas.height;\n\
  context.arc(x, y, 16, 0, 2 * Math.PI, false);\n\
  context.closePath();\n\
  context.fill();\n\
}\n\
\n\
function renderTestPatternLoop() {\n\
  updateTestPattern();\n\
  requestAnimationFrame(renderTestPatternLoop);\n\
}\n\
renderTestPatternLoop();\n\
\n' +
script + '\n\
</script></html>';
}

// Return the given |html| document as an encoded data URI.
function makeDataUriFromDocument(html) {
  return 'data:text/html;charset=UTF-8,' + encodeURIComponent(html);
}

// Returns capture options for starting the off-screen tab.
function getCaptureOptions() {
  return {
    video: true,
    audio: false,
    videoConstraints: {
      mandatory: {
        minWidth: width,
        minHeight: height,
        maxWidth: width,
        maxHeight: height,
        maxFrameRate: frameRate,
      }
    }
  };
}

// Samples the video frames from a capture |stream|, testing the color of the
// center pixel.  Once the pixel matches one of the colors in |expectedColors|,
// run |callback| with the index of the matched color.  |colorDeviation| is used
// to imprecisely match colors.
function waitForAnExpectedColor(
    stream, expectedColors, colorDeviation, callback) {
  chrome.test.assertTrue(!!stream);
  chrome.test.assertTrue(expectedColors.length > 0);
  chrome.test.assertTrue(!!callback);

  var video = document.getElementById('video');
  chrome.test.assertTrue(!!video);

  // If not yet done, plug the LocalMediaStream into the video element.
  if (!this.stream || this.stream != stream) {
    this.stream = stream;
    video.srcObject = stream;
    video.play();
  }

  // Create a canvas to sample frames from the video element.
  if (!this.canvas) {
    this.canvas = document.createElement("canvas");
    this.canvas.width = width;
    this.canvas.height = height;
  }

  // Only bother examining a video frame if the video timestamp has advanced.
  var currentVideoTimestamp = video.currentTime;
  if (!this.lastVideoTimestamp ||
      this.lastVideoTimestamp < currentVideoTimestamp) {
    this.lastVideoTimestamp = currentVideoTimestamp;

    // Grab a snapshot of the center pixel of the video.
    var ctx = this.canvas.getContext("2d");
    ctx.drawImage(video, 0, 0, width, height);
    var imageData = ctx.getImageData(width / 2, height / 2, 1, 1);
    var pixel = [imageData.data[0], imageData.data[1], imageData.data[2]];

    // Does the pixel match one of the expected colors?
    for (var i = 0, end = expectedColors.length; i < end; ++i) {
      var color = expectedColors[i];
      if (Math.abs(pixel[0] - color[0]) <= colorDeviation &&
          Math.abs(pixel[1] - color[1]) <= colorDeviation &&
          Math.abs(pixel[2] - color[2]) <= colorDeviation) {
        console.debug("Observed expected color RGB(" + color +
            ") in the video as RGB(" + pixel + ")");
        setTimeout(function () { callback(i); }, 0);
        return;
      }
    }
  }

  // At this point, an expected color has not been observed.  Schedule another
  // check after a short delay.
  setTimeout(
    function () {
      waitForAnExpectedColor(stream, expectedColors, colorDeviation, callback);
    },
    1000 / frameRate);
}

// Stops all the tracks in a MediaStream.
function stopAllTracks(stream) {
  var tracks = stream.getTracks();
  for (var i = 0, end = tracks.length; i < end; ++i) {
    tracks[i].stop();
  }
  chrome.test.assertFalse(stream.active);
}
