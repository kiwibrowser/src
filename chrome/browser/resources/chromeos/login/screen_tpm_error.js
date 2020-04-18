// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Offline message screen implementation.
 */

login.createScreen('TPMErrorMessageScreen', 'tpm-error-message', function() {
  return {
    EXTERNAL_API: ['show'],

    /**
     * Buttons in oobe wizard's button strip.
     * @type {array} Array of Buttons.
     */
    get buttons() {
      var rebootButton = this.ownerDocument.createElement('button');
      rebootButton.id = 'reboot-button';
      rebootButton.textContent =
          loadTimeData.getString('errorTpmFailureRebootButton');
      rebootButton.addEventListener('click', function() {
        chrome.send('rebootSystem');
      });
      return [rebootButton];
    },

    /**
     * Show TPM screen.
     */
    show: function() {
      Oobe.showScreen({id: SCREEN_TPM_ERROR});
    }
  };
});
