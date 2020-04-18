// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Javascript for local_state.html, served from chrome://local-state/
 * This is used to debug the contents of the Local State file.
 */

cr.define('localState', function() {
  'use strict';

  /**
   * Sets the page content to the specified |localState| string, called
   * from C++.
   * @param {string} localState the JSON-formatted local state data,
   *                 or an error message.
   */
  function setLocalState(localState) {
    $('content').textContent = localState;
  }

  return {setLocalState: setLocalState};
});

// When the page loads, request the JSON local state data from C++.
document.addEventListener('DOMContentLoaded', function() {
  chrome.send('requestJson');
});
