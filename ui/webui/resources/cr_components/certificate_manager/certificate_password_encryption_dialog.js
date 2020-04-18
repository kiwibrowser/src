// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview A dialog prompting the user to encrypt a personal certificate
 * before it is exported to disk.
 */
Polymer({
  is: 'certificate-password-encryption-dialog',

  behaviors: [I18nBehavior],

  properties: {
    /** @type {!CertificateSubnode} */
    model: Object,

    /** @private */
    password_: {
      type: String,
      value: '',
    },

    /** @private */
    confirmPassword_: {
      type: String,
      value: '',
    },
  },

  /** @private {?certificate_manager.CertificatesBrowserProxy} */
  browserProxy_: null,

  /** @override */
  ready: function() {
    this.browserProxy_ =
        certificate_manager.CertificatesBrowserProxyImpl.getInstance();
  },

  /** @override */
  attached: function() {
    /** @type {!CrDialogElement} */ (this.$.dialog).showModal();
  },

  /** @private */
  onCancelTap_: function() {
    /** @type {!CrDialogElement} */ (this.$.dialog).close();
  },

  /** @private */
  onOkTap_: function() {
    this.browserProxy_.exportPersonalCertificatePasswordSelected(this.password_)
        .then(
            () => {
              this.$.dialog.close();
            },
            error => {
              this.$.dialog.close();
              this.fire('certificates-error', {error: error, anchor: null});
            });
  },

  /** @private */
  validate_: function() {
    var isValid =
        this.password_ != '' && this.password_ == this.confirmPassword_;
    this.$.ok.disabled = !isValid;
  },
});
