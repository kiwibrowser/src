// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview A dialog for showing SSL certificate related error messages.
 * The user can only close the dialog, there is no other possible interaction.
 */
Polymer({
  is: 'certificates-error-dialog',

  behaviors: [I18nBehavior],

  properties: {
    /** @type {!CertificatesError|!CertificatesImportError} */
    model: Object,
  },

  /** @override */
  attached: function() {
    /** @type {!CrDialogElement} */ (this.$.dialog).showModal();
  },

  /** @private */
  onOkTap_: function() {
    /** @type {!CrDialogElement} */ (this.$.dialog).close();
  },

  /**
   * @param {{name: string, error: string}} importError
   * @return {string}
   * @private
   */
  getCertificateErrorText_: function(importError) {
    return loadTimeData.getStringF(
        'certificateImportErrorFormat', importError.name, importError.error);
  },
});
