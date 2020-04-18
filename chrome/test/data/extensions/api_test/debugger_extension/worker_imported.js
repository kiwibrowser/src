// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

self.addEventListener('connect', function(e) {
  var port = e.ports[0];
  port.start();
  port.postMessage({});
});