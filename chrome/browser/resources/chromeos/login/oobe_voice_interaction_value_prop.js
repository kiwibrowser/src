// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Polymer element for displaying material design voice
 * interaction value prop screen.
 */

Polymer({
  is: 'voice-interaction-value-prop-md',

  properties: {
    /**
     * Buttons are disabled when the value prop content is loading.
     */
    valuePropButtonsDisabled: {
      type: Boolean,
      value: true,
    },

    /**
     * System locale.
     */
    locale: {
      type: String,
    },

    /**
     * Default url for local en.
     */
    defaultUrl: {
      type: String,
      value:
          'https://www.gstatic.com/opa-android/oobe/a02187e41eed9e42/v1_omni_en_us.html',
    },
  },

  /**
   * Whether try to reload with the default url when a 404 error occurred.
   * @type {boolean}
   * @private
   */
  reloadWithDefaultUrl_: false,

  /**
   * Whether an error occurs while the webview is loading.
   * @type: {boolean}
   * @private
   */
  valuePropError_: false,

  /**
   * Timeout ID for loading animation.
   * @type {number}
   * @private
   */
  animationTimeout_: null,

  /**
   * Timeout ID for loading (will fire an error).
   * @type {number}
   * @private
   */
  loadingTimeout_: null,

  /**
   * The value prop view object.
   * @type {Object}
   * @private
   */
  valueView_: null,

  /**
   * Whether the screen has been initialized.
   * @type {boolean}
   * @private
   */
  initialized_: false,

  /**
   * Whether the response header has been received for the value prop view
   * @type: {boolean}
   * @private
   */
  headerReceived_: false,

  /**
   * On-tap event handler for skip button.
   *
   * @private
   */
  onSkipTap_: function() {
    chrome.send(
        'login.VoiceInteractionValuePropScreen.userActed', ['skip-pressed']);
  },

  /**
   * On-tap event handler for retry button.
   *
   * @private
   */
  onRetryTap_: function() {
    this.reloadValueProp();
  },

  /**
   * On-tap event handler for next button.
   *
   * @private
   */
  onNextTap_: function() {
    chrome.send(
        'login.VoiceInteractionValuePropScreen.userActed', ['next-pressed']);
  },

  /**
   * Add class to the list of classes of root elements.
   * @param {string} className class to add
   *
   * @private
   */
  addClass_: function(className) {
    this.$['voice-dialog'].classList.add(className);
  },

  /**
   * Remove class to the list of classes of root elements.
   * @param {string} className class to remove
   *
   * @private
   */
  removeClass_: function(className) {
    this.$['voice-dialog'].classList.remove(className);
  },

  /**
   * Reloads value prop.
   */
  reloadValueProp: function() {
    this.valuePropError_ = false;
    this.headerReceived_ = false;
    this.valueView_.src =
        'https://www.gstatic.com/opa-android/oobe/a02187e41eed9e42/v1_omni_' +
        this.locale + '.html';

    window.clearTimeout(this.animationTimeout_);
    window.clearTimeout(this.loadingTimeout_);
    this.removeClass_('value-prop-loaded');
    this.removeClass_('value-prop-error');
    this.addClass_('value-prop-loading');
    this.valuePropButtonsDisabled = true;

    this.animationTimeout_ = window.setTimeout(function() {
      this.addClass_('value-prop-loading-animation');
    }.bind(this), 500);
    this.loadingTimeout_ = window.setTimeout(function() {
      this.onValueViewErrorOccurred();
    }.bind(this), 5000);
  },

  /**
   * Handles event when value prop view cannot be loaded.
   */
  onValueViewErrorOccurred: function(details) {
    // TODO(updowndota): Remove after bug is fixed.
    console.error('Value prop view error: ' + JSON.stringify(details));
    this.valuePropError_ = true;
    window.clearTimeout(this.animationTimeout_);
    window.clearTimeout(this.loadingTimeout_);
    this.removeClass_('value-prop-loading-animation');
    this.removeClass_('value-prop-loading');
    this.removeClass_('value-prop-loaded');
    this.addClass_('value-prop-error');

    this.valuePropButtonsDisabled = false;
    this.$['retry-button'].focus();
  },

  /**
   * Handles event when value prop view is loaded.
   */
  onValueViewContentLoad: function(details) {
    // TODO(updowndota): Remove after bug is fixed.
    console.error('Value prop view loaded: ' + JSON.stringify(details));
    if (details == null) {
      return;
    }
    if (this.valuePropError_ || !this.headerReceived_) {
      return;
    }
    if (this.reloadWithDefaultUrl_) {
      this.valueView_.src = this.defaultUrl;
      this.headerReceived_ = false;
      this.reloadWithDefaultUrl_ = false;
      return;
    }

    window.clearTimeout(this.animationTimeout_);
    window.clearTimeout(this.loadingTimeout_);
    this.removeClass_('value-prop-loading-animation');
    this.removeClass_('value-prop-loading');
    this.removeClass_('value-prop-error');
    this.addClass_('value-prop-loaded');

    this.valuePropButtonsDisabled = false;
    this.$['next-button'].focus();
  },

  /**
   * Handles event when webview request headers received.
   */
  onValueViewHeadersReceived: function(details) {
    // TODO(updowndota): Remove after bug is fixed.
    console.error(
        'Value prop view header received: ' + JSON.stringify(details));
    if (details == null) {
      return;
    }
    this.headerReceived_ = true;
    if (details.statusCode == '404') {
      if (details.url != this.defaultUrl) {
        this.reloadWithDefaultUrl_ = true;
      } else {
        this.onValueViewErrorOccurred();
      }
    } else if (details.statusCode != '200') {
      this.onValueViewErrorOccurred();
    }
  },

  /**
   * Signal from host to show the screen.
   */
  onShow: function() {
    var requestFilter = {urls: ['<all_urls>'], types: ['main_frame']};
    this.valueView_ = this.$['value-prop-view'];
    this.locale = this.locale.replace('-', '_').toLowerCase();

    if (!this.initialized_) {
      this.valueView_.request.onErrorOccurred.addListener(
          this.onValueViewErrorOccurred.bind(this), requestFilter);
      this.valueView_.request.onHeadersReceived.addListener(
          this.onValueViewHeadersReceived.bind(this), requestFilter);
      this.valueView_.request.onCompleted.addListener(
          this.onValueViewContentLoad.bind(this), requestFilter);

      this.valueView_.addContentScripts([{
        name: 'stripLinks',
        matches: ['<all_urls>'],
        js: {
          code: 'document.querySelectorAll(\'a\').forEach(' +
              'function(anchor){anchor.href=\'javascript:void(0)\';})'
        },
        run_at: 'document_end'
      }]);

      this.initialized_ = true;
    }

    this.reloadValueProp();
  },
});
