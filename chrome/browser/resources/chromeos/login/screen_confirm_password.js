// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Password confirmation screen implementation.
 */

login.createScreen('ConfirmPasswordScreen', 'confirm-password', function() {
  return {
    EXTERNAL_API: ['show'],

    confirmPasswordForm_: null,

    /**
     * Callback to run when the screen is dismissed.
     * @type {function(string)}
     */
    callback_: null,

    /** @override */
    decorate: function() {
      this.confirmPasswordForm_ = $('saml-confirm-password');
      this.confirmPasswordForm_.addEventListener('cancel', function(e) {
        Oobe.showScreen({id: SCREEN_ACCOUNT_PICKER});
        Oobe.resetSigninUI(true);
      });
      this.confirmPasswordForm_.addEventListener('passwordEnter', function(e) {
        this.callback_(e.detail.password);
      }.bind(this));
    },

    /** @override */
    onBeforeShow: function(data) {
      $('login-header-bar').signinUIState =
          SIGNIN_UI_STATE.SAML_PASSWORD_CONFIRM;
    },

    /** @override */
    onAfterShow: function(data) {
      this.confirmPasswordForm_.focus();
    },

    /** @override */
    onBeforeHide: function() {
      this.confirmPasswordForm_.reset();
    },

    /**
     * Shows the confirm password screen.
     * @param {string} email The authenticated user's e-mail.
     * @param {boolean} manualPasswordInput True if no password has been
     *     scrapped and the user needs to set one manually for the device.
     * @param {number} attemptCount Number of attempts tried, starting at 0.
     * @param {function(string)} callback The callback to be invoked when the
     *     screen is dismissed.
     */
    show: function(email, manualPasswordInput, attemptCount, callback) {
      this.callback_ = callback;
      this.confirmPasswordForm_.reset();
      this.confirmPasswordForm_.email = email;
      this.confirmPasswordForm_.manualInput = manualPasswordInput;
      if (attemptCount > 0)
        this.confirmPasswordForm_.invalidate();
      Oobe.showScreen({id: SCREEN_CONFIRM_PASSWORD});
      $('progress-dots').hidden = true;
    }
  };
});
