// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var browserTarget = new content.mojom.BrowserTargetPtr;
Mojo.bindInterface(content.mojom.BrowserTarget.name,
                   mojo.makeRequest(browserTarget).handle);

browserTarget.start().then(function() {
  browserTarget.stop();
});