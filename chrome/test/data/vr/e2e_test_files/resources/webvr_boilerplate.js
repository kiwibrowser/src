// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Add additional setup steps to the object from webvr_e2e.js if it exists.
if (typeof initializationSteps !== "undefined") {
  initializationSteps["getVRDisplays"] = false;
} else {
  // Create here if it doesn't exist so we can access it later without checking
  // if it's defined.
  var initializationSteps = {};
}

var webglCanvas = document.getElementById("webgl-canvas");
var glAttribs = {
  alpha: false,
};
var gl = webglCanvas.getContext("webgl", glAttribs);
var vrDisplay = null;
var frameData = null;
var onMagicWindowAnimationFrameCallback = null;
var onPresentingAnimationFrameCallback = null;
var shouldSubmitFrame = true;
var hasPresentedFrame = false;

function onResize() {
  if (vrDisplay && vrDisplay.isPresenting) {
    var leftEye = vrDisplay.getEyeParameters("left");
    var rightEye = vrDisplay.getEyeParameters("right");

    webglCanvas.width = Math.max(leftEye.renderWidth, rightEye.renderWidth) * 2;
    webglCanvas.height = Math.max(leftEye.renderHeight, rightEye.renderHeight);
  } else {
    webglCanvas.width = webglCanvas.offsetWidth * window.devicePixelRatio;
    webglCanvas.height = webglCanvas.offsetHeight * window.devicePixelRatio;
  }
}

function onVrPresentChange() {
  onResize();
}

function onVrRequestPresent() {
  vrDisplay.requestPresent([{source: webglCanvas}]);
}

function onAnimationFrame(t) {
  if (vrDisplay == null) {
    window.requestAnimationFrame(onAnimationFrame);
    gl.viewport(0, 0, webglCanvas.width, webglCanvas.height);
    return;
  }
  vrDisplay.requestAnimationFrame(onAnimationFrame);
  // If presenting, set canvas to blue. Otherwise, red.
  if (vrDisplay.isPresenting) {
    if (onPresentingAnimationFrameCallback) {
      onPresentingAnimationFrameCallback();
    }
    vrDisplay.getFrameData(frameData);

    gl.clearColor(0.0, 0.0, 1.0, 1.0);
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

    gl.viewport(0, 0, webglCanvas.width * 0.5, webglCanvas.height);
    gl.viewport(webglCanvas.width * 0.5, 0, webglCanvas.width * 0.5,
                webglCanvas.height);

    if (shouldSubmitFrame) {
      vrDisplay.submitFrame();
      hasPresentedFrame = true;
    }
  } else {
    if (onMagicWindowAnimationFrameCallback) {
      onMagicWindowAnimationFrameCallback();
    }
    gl.clearColor(1.0, 0.0, 0.0, 1.0);
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

    gl.viewport(0, 0, webglCanvas.width, webglCanvas.height);
  }
}

if (navigator.getVRDisplays) {
  frameData = new VRFrameData();
  navigator.getVRDisplays().then( (displays) => {
    if (displays.length > 0) {
      vrDisplay = displays[0];
    }
  }).then( () => {
    initializationSteps["getVRDisplays"] = true;
  });
} else {
  initializationSteps["getVRDisplays"] = true;
}

gl.clearColor(1.0, 0.0, 0.0, 1.0);
gl.enable(gl.DEPTH_TEST);
gl.enable(gl.CULL_FACE);
window.addEventListener("resize", onResize, false);
window.addEventListener("vrdisplaypresentchange", onVrPresentChange, false);
window.requestAnimationFrame(onAnimationFrame);
webglCanvas.onclick = onVrRequestPresent;
onResize();
