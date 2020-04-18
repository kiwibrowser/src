// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('extensions', function() {
  'use strict';

  const PackDialogAlert = Polymer({
    is: 'extensions-pack-dialog-alert',
    properties: {
      /** @private {chrome.developerPrivate.PackDirectoryResponse} */
      model: Object,

      /** @private */
      title_: String,

      /** @private */
      message_: String,

      /** @private {?string} */
      cancelLabel_: String,

      /**
       * This needs to be initialized to trigger data-binding.
       * @private {?string}
       */
      confirmLabel_: {
        type: String,
        value: '',
      }
    },

    /** @return {string} */
    get returnValue() {
      return /** @type {!CrDialogElement} */ (this.$.dialog)
          .getNative()
          .returnValue;
    },

    /** @override */
    ready: function() {
      // Initialize button label values for initial html binding.
      this.cancelLabel_ = null;
      this.confirmLabel_ = null;

      switch (this.model.status) {
        case chrome.developerPrivate.PackStatus.WARNING:
          this.title_ = loadTimeData.getString('packDialogWarningTitle');
          this.cancelLabel_ = loadTimeData.getString('cancel');
          this.confirmLabel_ =
              loadTimeData.getString('packDialogProceedAnyway');
          break;
        case chrome.developerPrivate.PackStatus.ERROR:
          this.title_ = loadTimeData.getString('packDialogErrorTitle');
          this.cancelLabel_ = loadTimeData.getString('ok');
          break;
        case chrome.developerPrivate.PackStatus.SUCCESS:
          this.title_ = loadTimeData.getString('packDialogTitle');
          this.cancelLabel_ = loadTimeData.getString('ok');
          break;
        default:
          assertNotReached();
          return;
      }
    },

    /** @override */
    attached: function() {
      this.$.dialog.showModal();
    },

    /**
     * @return {string}
     * @private
     */
    getCancelButtonClass_: function() {
      return this.confirmLabel_ ? 'cancel-button' : 'action-button';
    },

    /** @private */
    onCancelTap_: function() {
      this.$.dialog.cancel();
    },

    /** @private */
    onConfirmTap_: function() {
      // The confirm button should only be available in WARNING state.
      assert(this.model.status === chrome.developerPrivate.PackStatus.WARNING);
      this.$.dialog.close();
    }
  });

  return {PackDialogAlert: PackDialogAlert};
});
