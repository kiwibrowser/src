// Copyright 2018 The Immersive Web Community Group
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

import {mat4} from '../math/gl-matrix.js';

const LOOK_SPEED = 0.0025;

export class FallbackHelper {
  constructor(scene, gl) {
    this.scene = scene;
    this.gl = gl;
    this._emulateStage = false;

    this.lookYaw = 0;
    this.lookPitch = 0;

    this.viewMatrix = mat4.create();

    let projectionMatrix = mat4.create();
    this.projectionMatrix = projectionMatrix;

    // Using a simple identity matrix for the view.
    mat4.identity(this.viewMatrix);

    // We need to track the canvas size in order to resize the WebGL
    // backbuffer width and height, as well as update the projection matrix
    // and adjust the viewport.
    function onResize() {
      gl.canvas.width = gl.canvas.offsetWidth * window.devicePixelRatio;
      gl.canvas.height = gl.canvas.offsetHeight * window.devicePixelRatio;
      mat4.perspective(projectionMatrix, Math.PI*0.4,
                       gl.canvas.width/gl.canvas.height,
                       0.1, 1000.0);
      gl.viewport(0, 0, gl.drawingBufferWidth, gl.drawingBufferHeight);
    }
    window.addEventListener('resize', onResize);
    onResize();

    // Upding the view matrix with touch or mouse events.
    let canvas = gl.canvas;
    let lastTouchX = 0;
    let lastTouchY = 0;
    canvas.addEventListener('touchstart', (ev) => {
      if (ev.touches.length == 2) {
        lastTouchX = ev.touches[1].pageX;
        lastTouchY = ev.touches[1].pageY;
      }
    });
    canvas.addEventListener('touchmove', (ev) => {
      // Rotate the view when two fingers are being used.
      if (ev.touches.length == 2) {
        this.onLook(ev.touches[1].pageX - lastTouchX, ev.touches[1].pageY - lastTouchY);
        lastTouchX = ev.touches[1].pageX;
        lastTouchY = ev.touches[1].pageY;
      }
    });
    canvas.addEventListener('mousemove', (ev) => {
      // Only rotate when the right button is pressed.
      if (ev.buttons & 2) {
        this.onLook(ev.movementX, ev.movementY);
      }
    });
    canvas.addEventListener('contextmenu', (ev) => {
      // Prevent context menus on the canvas so that we can use right click to rotate.
      ev.preventDefault();
    });

    this.boundOnFrame = this.onFrame.bind(this);
    window.requestAnimationFrame(this.boundOnFrame);
  }

  onLook(yaw, pitch) {
    this.lookYaw += yaw * LOOK_SPEED;
    this.lookPitch += pitch * LOOK_SPEED;

    // Clamp pitch rotation beyond looking straight up or down.
    if (this.lookPitch < -Math.PI*0.5) {
        this.lookPitch = -Math.PI*0.5;
    }
    if (this.lookPitch > Math.PI*0.5) {
        this.lookPitch = Math.PI*0.5;
    }

    this.updateView();
  }

  onFrame(t) {
    let gl = this.gl;
    window.requestAnimationFrame(this.boundOnFrame);

    this.scene.startFrame();

    // We can skip setting the framebuffer and viewport every frame, because
    // it won't change from frame to frame and we're updating the viewport
    // only when we resize for efficency.
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

    // We're drawing with our own projection and view matrix now, and we
    // don't have a list of view to loop through, but otherwise all of the
    // WebGL drawing logic is exactly the same.
    this.scene.draw(this.projectionMatrix, this.viewMatrix);

    this.scene.endFrame();
  }

  get emulateStage() {
    return this._emulateStage;
  }

  set emulateStage(value) {
    this._emulateStage = value;
    this.updateView();
  }

  updateView() {
    mat4.identity(this.viewMatrix);

    mat4.rotateX(this.viewMatrix, this.viewMatrix, -this.lookPitch);
    mat4.rotateY(this.viewMatrix, this.viewMatrix, -this.lookYaw);

    // If we're emulating a stage frame of reference we'll need to move the view
    // matrix roughly a meter and a half up in the air.
    if (this._emulateStage) {
      mat4.translate(this.viewMatrix, this.viewMatrix, [0, -1.6, 0]);
    }
  }
}
