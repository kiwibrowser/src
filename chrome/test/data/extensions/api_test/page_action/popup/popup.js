// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Wait to be resized by the browser once before indicating pass.
function onResize() {
  window.removeEventListener("resize", onResize);
  chrome.test.notifyPass();
}

window.addEventListener("resize", onResize);
