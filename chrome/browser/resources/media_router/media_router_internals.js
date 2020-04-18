// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Handles user events for the Media Router Internals UI.
cr.define('media_router_internals', function() {
  'use strict';

  /**
   * Initializes the Media Router Internals WebUI
   */
  function initialize() {
    // Notify the browser that the page has loaded, causing it to send media
    // router status.
    chrome.send('initialized');
  }

  function setStatus(status) {
    const jsonStatus = JSON.stringify(status, null, /* spacing level = */ 2);
    $('sink-status-div').textContent = jsonStatus;
  }

  return {
    initialize: initialize,
    setStatus: setStatus,
  };
});

document.addEventListener(
    'DOMContentLoaded', media_router_internals.initialize);
