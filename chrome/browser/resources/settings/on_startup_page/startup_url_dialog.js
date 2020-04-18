// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

/**
 * Describe the current URL input error status.
 * @enum {number}
 */
let UrlInputError = {
  NONE: 0,
  INVALID_URL: 1,
  TOO_LONG: 2,
};

/**
 * @fileoverview 'settings-startup-url-dialog' is a component for adding
 * or editing a startup URL entry.
 */
Polymer({
  is: 'settings-startup-url-dialog',

  properties: {
    /** @private {UrlInputError} */
    error_: {
      type: Number,
      value: UrlInputError.NONE,
    },

    /** @private */
    url_: String,

    /** @private */
    urlLimit_: {
      readOnly: true,
      type: Number,
      value: 100 * 1024,  // 100 KB.
    },

    /**
     * If specified the dialog acts as an "Edit page" dialog, otherwise as an
     * "Add new page" dialog.
     * @type {?StartupPageInfo}
     */
    model: Object,

    /** @private */
    dialogTitle_: String,

    /** @private */
    actionButtonText_: String,
  },

  /** @private {!settings.SearchEnginesBrowserProxy} */
  browserProxy_: null,

  /** @override */
  attached: function() {
    this.browserProxy_ = settings.StartupUrlsPageBrowserProxyImpl.getInstance();

    if (this.model) {
      this.dialogTitle_ = loadTimeData.getString('onStartupEditPage');
      this.actionButtonText_ = loadTimeData.getString('save');
      this.$.actionButton.disabled = false;
      // Pre-populate the input field.
      this.url_ = this.model.url;
    } else {
      this.dialogTitle_ = loadTimeData.getString('onStartupAddNewPage');
      this.actionButtonText_ = loadTimeData.getString('add');
      this.$.actionButton.disabled = true;
    }
    this.$.dialog.showModal();
  },

  /**
   * @param {string} invalidUrl
   * @param {string} tooLong
   * @return {string}
   * @private
   */
  errorMessage_: function(invalidUrl, tooLong) {
    return ['', invalidUrl, tooLong][this.error_];
  },

  /** @private */
  onCancelTap_: function() {
    this.$.dialog.close();
  },

  /** @private */
  onActionButtonTap_: function() {
    const whenDone = this.model ?
        this.browserProxy_.editStartupPage(this.model.modelIndex, this.url_) :
        this.browserProxy_.addStartupPage(this.url_);

    whenDone.then(success => {
      if (success)
        this.$.dialog.close();
      // If the URL was invalid, there is nothing to do, just leave the dialog
      // open and let the user fix the URL or cancel.
    });
  },

  /** @private */
  showCharCounter_: function() {
    return this.error_ == UrlInputError.TOO_LONG;
  },

  /** @private */
  validate_: function() {
    if (this.url_.length == 0) {
      this.$.actionButton.disabled = true;
      this.error_ = UrlInputError.NONE;
      return;
    }
    if (this.url_.length >= this.urlLimit_) {
      this.$.actionButton.disabled = true;
      this.error_ = UrlInputError.TOO_LONG;
      return;
    }
    this.browserProxy_.validateStartupPage(this.url_).then(isValid => {
      this.$.actionButton.disabled = !isValid;
      this.error_ = isValid ? UrlInputError.NONE : UrlInputError.INVALID_URL;
    });
  },
});
})();
