// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('extensions', function() {
  'use strict';

  /** @interface */
  class LoadErrorDelegate {
    /**
     * Attempts to load the previously-attempted unpacked extension.
     * @param {string} retryId
     * @return {!Promise}
     */
    retryLoadUnpacked(retryId) {}
  }

  const LoadError = Polymer({
    is: 'extensions-load-error',
    properties: {
      /** @type {extensions.LoadErrorDelegate} */
      delegate: Object,

      /** @type {chrome.developerPrivate.LoadError} */
      loadError: Object,

      /** @private */
      retrying_: Boolean,
    },

    observers: [
      'observeLoadErrorChanges_(loadError)',
    ],

    show: function() {
      /** @type {!CrDialogElement} */ (this.$.dialog).showModal();
    },

    close: function() {
      /** @type {!CrDialogElement} */ (this.$.dialog).close();
    },

    /** @private */
    onRetryTap_: function() {
      this.retrying_ = true;
      this.delegate.retryLoadUnpacked(this.loadError.retryGuid)
          .then(
              () => {
                this.close();
              },
              loadError => {
                this.loadError =
                    /** @type {chrome.developerPrivate.LoadError} */ (
                        loadError);
                this.retrying_ = false;
              });
    },

    /** @private */
    observeLoadErrorChanges_: function() {
      assert(this.loadError);
      const source = this.loadError.source;
      // CodeSection expects a RequestFileSourceResponse, rather than an
      // ErrorFileSource. Massage into place.
      // TODO(devlin): Make RequestFileSourceResponse use ErrorFileSource.
      /** @type {!chrome.developerPrivate.RequestFileSourceResponse} */
      const codeSectionProperties = {
        beforeHighlight: source ? source.beforeHighlight : '',
        highlight: source ? source.highlight : '',
        afterHighlight: source ? source.afterHighlight : '',
        title: '',
        message: this.loadError.error,
      };

      this.$.code.code = codeSectionProperties;
    },
  });

  return {LoadError: LoadError, LoadErrorDelegate: LoadErrorDelegate};
});
