// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Add additional setup steps to the object from webvr_e2e.js if it exists.
if (typeof initializationSteps !== 'undefined') {
  initializationSteps['getXRDevice'] = false;
  initializationSteps['magicWindowStarted'] = false;
} else {
  // Create here if it doesn't exist so we can access it later without checking
  // if it's defined.
  var initializationSteps = {};
}

var webglCanvas = document.getElementById('webgl-canvas');
var glAttribs = {
  alpha: false,
};
var gl = null;
var xrDevice = null;
var onMagicWindowXRFrameCallback = null;
var onExclusiveXRFrameCallback = null;
var shouldSubmitFrame = true;
var hasPresentedFrame = false;

var magicWindowSession = null;
var magicWindowFrameOfRef = null;
var exclusiveSession = null;
var exclusiveFrameOfRef = null;

function onRequestSession() {
  xrDevice.requestSession({exclusive: true}).then( (session) => {
    exclusiveSession = session;
    onSessionStarted(session);
  });
}

function onSessionStarted(session) {
  session.addEventListener('end', onSessionEnded);
  // Initialize the WebGL context for use with XR if it hasn't been already
  if (!gl) {
    glAttribs['compatibleXRDevice'] = session.device;

    // Create an offscreen canvas and get its context
    let offscreenCanvas = document.createElement('canvas');
    gl = offscreenCanvas.getContext('webgl', glAttribs);
    if (!gl) {
      console.error('Failed to get WebGL context');
    }
    gl.clearColor(0.0, 1.0, 0.0, 1.0);
    gl.enable(gl.DEPTH_TEST);
    gl.enable(gl.CULL_FACE);
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
  }

  session.baseLayer = new XRWebGLLayer(session, gl);
  session.requestFrameOfReference('eyeLevel').then( (frameOfRef) => {
    if (session.exclusive) {
      exclusiveFrameOfRef = frameOfRef;
    } else {
      magicWindowFrameOfRef = frameOfRef;
    }
    session.requestAnimationFrame(onXRFrame);
  });
}

function onSessionEnded(event) {
  if (event.session.exclusive)
    exclusiveSession = null
  else
    magicWindowSession = null;
}

function onXRFrame(t, frame) {
  let session = frame.session;
  session.requestAnimationFrame(onXRFrame);
  let frameOfRef = session.exclusive ?
                   exclusiveFrameOfRef :
                   magicWindowFrameOfRef;
  let pose = frame.getDevicePose(frameOfRef);

  // Exiting the rAF callback without dirtying the GL context is interpreted
  // as not submitting a frame
  if (!shouldSubmitFrame) {
    return;
  }

  gl.bindFramebuffer(gl.FRAMEBUFFER, session.baseLayer.framebuffer);

  // If in an exclusive session, set canvas to blue. Otherwise, red.
  if (session.exclusive) {
    if (onExclusiveXRFrameCallback) {
      onExclusiveXRFrameCallback(session, frame);
    }
    gl.clearColor(0.0, 0.0, 1.0, 1.0);
  } else {
    if (onMagicWindowXRFrameCallback) {
      onMagicWindowXRFrameCallback(session, frame);
    }
    gl.clearColor(1.0, 0.0, 0.0, 1.0);
  }
  gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
  hasPresentedFrame = true;
}

// Try to get an XRDevice and set up a non-exclusive session with it
if (navigator.xr) {
  navigator.xr.requestDevice().then( (device) => {
    xrDevice = device;
    if (!device)
      return;

    // Set up the device to have a non-exclusive session (magic window) drawing
    // into the full screen canvas on the page
    let ctx = webglCanvas.getContext('xrpresent');
    device.requestSession({outputContext: ctx}).then( (session) => {
      onSessionStarted(session);
    }).then( () => {
      initializationSteps['magicWindowStarted'] = true;
    });
  }).then( () => {
    initializationSteps['getXRDevice'] = true;
  });
} else {
  initializationSteps['getXRDevice'] = true;
  initializationSteps['magicWindowStarted'] = true;
}

webglCanvas.onclick = onRequestSession;
