/* Copyright 2018 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. */

'use strict';
/**
 * JavaScript for reset_password.html, served from chrome://reset-password/.
 */
(function() {

/** @type {mojom.ResetPasswordHandler} */
var uiHandler;

function initialize() {
  uiHandler = new mojom.ResetPasswordHandlerPtr;
  Mojo.bindInterface(
      mojom.ResetPasswordHandler.name, mojo.makeRequest(uiHandler).handle);

  /** @type {?HTMLElement} */
  var resetPasswordButton = $('reset-password-button');
  resetPasswordButton.addEventListener('click', function() {
    uiHandler.handlePasswordReset();
  });
}

document.addEventListener('DOMContentLoaded', initialize);
})();
