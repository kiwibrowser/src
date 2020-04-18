// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview A dialog prompting the user for a decryption password such that
 * a previously exported personal certificate can be imported.
 */
Polymer({
  is: 'certificate-password-decryption-dialog',

  behaviors: [I18nBehavior],

  properties: {
    /** @private */
    password_: {
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
    this.browserProxy_.importPersonalCertificatePasswordSelected(this.password_)
        .then(
            () => {
              /** @type {!CrDialogElement} */ (this.$.dialog).close();
            },
            error => {
              /** @type {!CrDialogElement} */ (this.$.dialog).close();
              this.fire('certificates-error', {error: error, anchor: null});
            });
  },
});
