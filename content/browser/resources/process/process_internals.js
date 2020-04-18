// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

console.log('process internals initializing');

(function() {
'use strict';

/**
 * Reference to the backend.
 * @type {mojom.ProcessInternalsHandlerPtr}
 */
var uiHandler = null;

document.addEventListener('DOMContentLoaded', function() {
  // Setup Mojo interface to the backend.
  uiHandler = new mojom.ProcessInternalsHandlerPtr;
  Mojo.bindInterface(
      mojom.ProcessInternalsHandler.name,
      mojo.makeRequest(uiHandler).handle);

  // Get the Site Isolation mode and populate it.
  uiHandler.getIsolationMode().then((response) => {
    document.getElementById('isolation-mode').innerText = response.mode;
  });
  uiHandler.getIsolatedOrigins().then((response) => {
    document.getElementById('isolated-origins').innerText =
        response.isolatedOrigins.join(', ');
  });
});

})();
