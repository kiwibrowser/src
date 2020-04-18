// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('reset', function() {

  var USER_ACTION_RESET_CONFIRM_DISMISSED = 'reset-confirm-dismissed';
  /**
   * ResetScreenConfirmationOverlay class
   * Encapsulated handling of the 'Confirm reset device' overlay OOBE page.
   * @class
   */
  function ConfirmResetOverlay() {}

  cr.addSingletonGetter(ConfirmResetOverlay);

  ConfirmResetOverlay.prototype = {
    /**
     * Initialize the page.
     */
    initializePage: function() {
      var overlay = $('reset-confirm-overlay');
      overlay.addEventListener('cancelOverlay', function(e) {
        $('reset').send(
            login.Screen.CALLBACK_USER_ACTED,
            USER_ACTION_RESET_CONFIRM_DISMISSED);
        e.stopPropagation();
      });
      $('overlay-reset').removeAttribute('hidden');
    },
  };

  // Export
  return {ConfirmResetOverlay: ConfirmResetOverlay};
});
