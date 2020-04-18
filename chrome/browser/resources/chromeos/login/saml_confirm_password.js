// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'saml-confirm-password',

  properties: {
    email: String,

    disabled: {type: Boolean, value: false, observer: 'disabledChanged_'},

    manualInput:
        {type: Boolean, value: false, observer: 'manualInputChanged_'}
  },

  ready: function() {
    /**
     * Workaround for
     * https://github.com/PolymerElements/neon-animation/issues/32
     * TODO(dzhioev): Remove when fixed in Polymer.
     */
    var pages = this.$.animatedPages;
    delete pages._squelchNextFinishEvent;
    Object.defineProperty(pages, '_squelchNextFinishEvent', {
      get: function() {
        return false;
      }
    });
  },

  reset: function() {
    if (this.$.cancelConfirmDlg.open)
      this.$.cancelConfirmDlg.close();
    this.disabled = false;
    this.$.navigation.closeVisible = true;
    if (this.$.animatedPages.selected != 0)
      this.$.animatedPages.selected = 0;
    this.$.passwordInput.isInvalid = false;
    this.$.passwordInput.value = '';
    if (this.manualInput) {
      this.$$('#confirmPasswordInput').isInvalid = false;
      this.$$('#confirmPasswordInput').value = '';
    }
  },

  invalidate: function() {
    this.$.passwordInput.isInvalid = true;
  },

  focus: function() {
    if (this.$.animatedPages.selected == 0)
      this.$.passwordInput.focus();
  },

  onClose_: function() {
    this.disabled = true;
    this.$.cancelConfirmDlg.showModal();
  },

  onCancelNo_: function() {
    this.$.cancelConfirmDlg.close();
  },

  onCancelYes_: function() {
    this.$.cancelConfirmDlg.close();
    this.fire('cancel');
  },

  onPasswordSubmitted_: function() {
    if (!this.$.passwordInput.checkValidity())
      return;
    if (this.manualInput) {
      // When using manual password entry, both passwords must match.
      var confirmPasswordInput = this.$$('#confirmPasswordInput');
      if (!confirmPasswordInput.checkValidity())
        return;

      if (confirmPasswordInput.value != this.$.passwordInput.value) {
        this.$.passwordInput.isInvalid = true;
        confirmPasswordInput.isInvalid = true;
        return;
      }
    }

    this.$.animatedPages.selected = 1;
    this.$.navigation.closeVisible = false;
    this.fire('passwordEnter', {password: this.$.passwordInput.value});
  },

  onDialogOverlayClosed_: function() {
    this.disabled = false;
  },

  disabledChanged_: function(disabled) {
    this.$.confirmPasswordCard.classList.toggle('full-disabled', disabled);
  },

  onAnimationFinish_: function() {
    if (this.$.animatedPages.selected == 1)
      this.$.passwordInput.value = '';
  },

  manualInputChanged_: function() {
    var titleId =
        this.manualInput ? 'manualPasswordTitle' : 'confirmPasswordTitle';
    var passwordInputLabelId =
        this.manualInput ? 'manualPasswordInputLabel' : 'confirmPasswordLabel';
    var passwordInputErrorId = this.manualInput ?
        'manualPasswordMismatch' :
        'confirmPasswordIncorrectPassword';

    this.$.title.textContent = loadTimeData.getString(titleId);
    this.$.passwordInput.label = loadTimeData.getString(passwordInputLabelId);
    this.$.passwordInput.error = loadTimeData.getString(passwordInputErrorId);
  },

  getConfirmPasswordInputLabel_: function() {
    return loadTimeData.getString('confirmPasswordLabel');
  },

  getConfirmPasswordInputError_: function() {
    return loadTimeData.getString('manualPasswordMismatch');
  }
});
