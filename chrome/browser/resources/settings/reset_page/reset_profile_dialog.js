// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 *
 * 'settings-reset-profile-dialog' is the dialog shown for clearing profile
 * settings. A triggered variant of this dialog can be shown under certain
 * circumstances. See triggered_profile_resetter.h for when the triggered
 * variant will be used.
 */
Polymer({
  is: 'settings-reset-profile-dialog',

  behaviors: [WebUIListenerBehavior],

  properties: {
    // TODO(dpapad): Evaluate whether this needs to be synced across different
    // settings tabs.

    /** @private */
    isTriggered_: {
      type: Boolean,
      value: false,
    },

    /** @private */
    triggeredResetToolName_: {
      type: String,
      value: '',
    },

    /** @private */
    resetRequestOrigin_: String,

    /** @private */
    clearingInProgress_: {
      type: Boolean,
      value: false,
    },
  },

  /** @private {?settings.ResetBrowserProxy} */
  browserProxy_: null,

  /**
   * @private
   * @return {string}
   */
  getExplanationText_: function() {
    if (this.isTriggered_) {
      return loadTimeData.getStringF(
          'triggeredResetPageExplanation', this.triggeredResetToolName_);
    }
    return loadTimeData.getStringF('resetPageExplanation');
  },

  /**
   * @private
   * @return {string}
   */
  getPageTitle_: function() {
    if (this.isTriggered_) {
      return loadTimeData.getStringF(
          'triggeredResetPageTitle', this.triggeredResetToolName_);
    }
    return loadTimeData.getStringF('resetDialogCommit');
  },

  /** @override */
  ready: function() {
    this.browserProxy_ = settings.ResetBrowserProxyImpl.getInstance();

    this.addEventListener('cancel', () => {
      this.browserProxy_.onHideResetProfileDialog();
    });

    this.$$('cr-checkbox a')
        .addEventListener('click', this.onShowReportedSettingsTap_.bind(this));
  },

  /** @private */
  showDialog_: function() {
    if (!this.$.dialog.open)
      this.$.dialog.showModal();
    this.browserProxy_.onShowResetProfileDialog();
  },

  show: function() {
    this.isTriggered_ =
        settings.getCurrentRoute() == settings.routes.TRIGGERED_RESET_DIALOG;
    if (this.isTriggered_) {
      this.browserProxy_.getTriggeredResetToolName().then(name => {
        this.resetRequestOrigin_ = 'triggeredreset';
        this.triggeredResetToolName_ = name;
        this.showDialog_();
      });
    } else {
      // For the non-triggered reset dialog, a '#cct' hash indicates that the
      // reset request came from the Chrome Cleanup Tool by launching Chrome
      // with the startup URL chrome://settings/resetProfileSettings#cct.
      const origin = window.location.hash.slice(1).toLowerCase() == 'cct' ?
          'cct' :
          settings.getQueryParameters().get('origin');
      this.resetRequestOrigin_ = origin || '';
      this.showDialog_();
    }
  },

  /** @private */
  onCancelTap_: function() {
    this.cancel();
  },

  cancel: function() {
    if (this.$.dialog.open)
      this.$.dialog.cancel();
  },

  /** @private */
  onResetTap_: function() {
    this.clearingInProgress_ = true;
    this.browserProxy_
        .performResetProfileSettings(
            this.$.sendSettings.checked, this.resetRequestOrigin_)
        .then(() => {
          this.clearingInProgress_ = false;
          if (this.$.dialog.open)
            this.$.dialog.close();
          this.fire('reset-done');
        });
  },

  /**
   * Displays the settings that will be reported in a new tab.
   * @param {!Event} e
   * @private
   */
  onShowReportedSettingsTap_: function(e) {
    this.browserProxy_.showReportedSettings();
    e.stopPropagation();
  },
});
