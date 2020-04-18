// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

window.addEventListener('resize', function(e) {
  if (!embedder) {
    return;
  }
  var msg = ['resize', document.body.clientWidth, document.body.clientHeight];
  embedder.postMessage(JSON.stringify(msg), '*');
});
