// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
'use strict';

/**
 * Creates a sample canvas.
 * @return {HTMLCanvasElement}
 */
function getSampleCanvas() {
  var canvas =
      /** @type {HTMLCanvasElement} */ (document.createElement('canvas'));
  canvas.width = 1920;
  canvas.height = 1080;

  var ctx = canvas.getContext('2d');
  ctx.fillStyle = '#000000';
  for (var i = 0; i < 10; i++) {
    ctx.fillRect(i * 30, i * 30, 20, 20);
  }

  return canvas;
}
