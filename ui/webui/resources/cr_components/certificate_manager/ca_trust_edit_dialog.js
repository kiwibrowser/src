// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'ca-trust-edit-dialog' allows the user to:
 *  - specify the trust level of a certificate authority that is being
 *    imported.
 *  - edit the trust level of an already existing certificate authority.
 */
Polymer({
  is: 'ca-trust-edit-dialog',

  behaviors: [I18nBehavior],

  properties: {
    /** @type {!CertificateSubnode|!NewCertificateSubNode} */
    model: Object,

    /** @private {?CaTrustInfo} */
    trustInfo_: Object,

    /** @private {string} */
    explanationText_: String,
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
    this.explanationText_ = loadTimeData.getStringF(
        'certificateManagerCaTrustEditDialogExplanation', this.model.name);

    // A non existing |model.id| indicates that a new certificate is being
    // imported, otherwise an existing certificate is being edited.
    if (this.model.id) {
      this.browserProxy_.getCaCertificateTrust(this.model.id)
          .then(trustInfo => {
            this.trustInfo_ = trustInfo;
            this.$.dialog.showModal();
          });
    } else {
      /** @type {!CrDialogElement} */ (this.$.dialog).showModal();
    }
  },

  /** @private */
  onCancelTap_: function() {
    /** @type {!CrDialogElement} */ (this.$.dialog).close();
  },

  /** @private */
  onOkTap_: function() {
    this.$.spinner.active = true;

    var whenDone = this.model.id ?
        this.browserProxy_.editCaCertificateTrust(
            this.model.id, this.$.ssl.checked, this.$.email.checked,
            this.$.objSign.checked) :
        this.browserProxy_.importCaCertificateTrustSelected(
            this.$.ssl.checked, this.$.email.checked, this.$.objSign.checked);

    whenDone.then(
        () => {
          this.$.spinner.active = false;
          /** @type {!CrDialogElement} */ (this.$.dialog).close();
        },
        error => {
          /** @type {!CrDialogElement} */ (this.$.dialog).close();
          this.fire('certificates-error', {error: error, anchor: null});
        });
  },
});
