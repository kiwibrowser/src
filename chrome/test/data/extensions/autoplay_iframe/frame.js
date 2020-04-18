// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

window.addEventListener('message', function() {
  var audio = document.createElement('audio');
  audio.src = 'test.mp4';
  audio.play().then(() => {
    top.postMessage('autoplayed', '*');
  }, e => {
    top.postMessage(e.name, '*');
  });
});
