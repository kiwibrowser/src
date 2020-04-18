/* Copyright 2017 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. */

cr.define('sync.confirmation', function() {
  'use strict';

  function initialize() {
    const syncConfirmationBrowserProxy =
        sync.confirmation.SyncConfirmationBrowserProxyImpl.getInstance();
    // Prefer using |document.body.offsetHeight| instead of
    // |document.body.scrollHeight| as it returns the correct height of the
    // even when the page zoom in Chrome is different than 100%.
    syncConfirmationBrowserProxy.initializedWithSize(
        [document.body.offsetHeight]);
  }

  function clearFocus() {
    document.activeElement.blur();
  }

  // The C++ handler calls out to this Javascript function, so it needs to
  // exist in the namespace. However, this version of the sync confirmation
  // doesn't use a user image, so we do not need to actually implement this.
  // TODO(scottchen): make the C++ handler not call this at all.
  function setUserImageURL() {}

  return {
    clearFocus: clearFocus,
    initialize: initialize,
    setUserImageURL: setUserImageURL
  };
});

document.addEventListener('DOMContentLoaded', sync.confirmation.initialize);