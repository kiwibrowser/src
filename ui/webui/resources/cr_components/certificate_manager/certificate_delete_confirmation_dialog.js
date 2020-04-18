// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview A confirmation dialog allowing the user to delete various types
 * of certificates.
 */
Polymer({
  is: 'certificate-delete-confirmation-dialog',

  behaviors: [I18nBehavior],

  properties: {
    /** @type {!CertificateSubnode} */
    model: Object,

    /** @type {!CertificateType} */
    certificateType: String,
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

  /**
   * @private
   * @return {string}
   */
  getTitleText_: function() {
    /**
     * @param {string} localizedMessageId
     * @return {string}
     */
    var getString = localizedMessageId =>
        loadTimeData.getStringF(localizedMessageId, this.model.name);

    switch (this.certificateType) {
      case CertificateType.PERSONAL:
        return getString('certificateManagerDeleteUserTitle');
      case CertificateType.SERVER:
        return getString('certificateManagerDeleteServerTitle');
      case CertificateType.CA:
        return getString('certificateManagerDeleteCaTitle');
      case CertificateType.OTHER:
        return getString('certificateManagerDeleteOtherTitle');
    }
    assertNotReached();
  },

  /**
   * @private
   * @return {string}
   */
  getDescriptionText_: function() {
    var getString = loadTimeData.getString.bind(loadTimeData);
    switch (this.certificateType) {
      case CertificateType.PERSONAL:
        return getString('certificateManagerDeleteUserDescription');
      case CertificateType.SERVER:
        return getString('certificateManagerDeleteServerDescription');
      case CertificateType.CA:
        return getString('certificateManagerDeleteCaDescription');
      case CertificateType.OTHER:
        return '';
    }
    assertNotReached();
  },

  /** @private */
  onCancelTap_: function() {
    /** @type {!CrDialogElement} */ (this.$.dialog).close();
  },

  /** @private */
  onOkTap_: function() {
    this.browserProxy_.deleteCertificate(this.model.id)
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
