// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function testWindowOpen(url) {
  window.open(url);
  window.domAutomationController.send(true);
}
