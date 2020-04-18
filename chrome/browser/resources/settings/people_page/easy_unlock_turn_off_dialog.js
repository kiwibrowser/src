// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview A dialog allowing the user to turn off the Easy Unlock feature.
 */

cr.exportPath('settings');

/**
 * Possible UI statuses for the EasyUnlockTurnOffDialogElement.
 * See easy_unlock_settings_handler.cc.
 * @enum {string}
 */
settings.EasyUnlockTurnOffStatus = {
  UNKNOWN: 'unknown',
  OFFLINE: 'offline',
  IDLE: 'idle',
  PENDING: 'pending',
  SERVER_ERROR: 'server-error',
};

(function() {

Polymer({
  is: 'easy-unlock-turn-off-dialog',

  behaviors: [I18nBehavior, WebUIListenerBehavior],

  properties: {
    /** @private {!settings.EasyUnlockTurnOffStatus} */
    status_: {
      type: String,
      value: settings.EasyUnlockTurnOffStatus.UNKNOWN,
    },
  },

  /** @private {settings.EasyUnlockBrowserProxy} */
  browserProxy_: null,

  /** @override */
  attached: function() {
    this.browserProxy_ = settings.EasyUnlockBrowserProxyImpl.getInstance();

    this.addWebUIListener(
        'easy-unlock-enabled-status',
        this.handleEasyUnlockEnabledStatusChanged_.bind(this));

    this.addWebUIListener('easy-unlock-turn-off-flow-status', status => {
      this.status_ = status;
    });

    // Since the dialog text depends on the status, defer opening until we have
    // retrieved the turn off status to prevent UI flicker.
    this.getTurnOffStatus_().then(status => {
      this.status_ = status;
      this.$.dialog.showModal();
    });
  },

  /**
   * @return {!Promise<!settings.EasyUnlockTurnOffStatus>}
   * @private
   */
  getTurnOffStatus_: function() {
    return navigator.onLine ?
        this.browserProxy_.getTurnOffFlowStatus() :
        Promise.resolve(settings.EasyUnlockTurnOffStatus.OFFLINE);
  },

  /**
   * This dialog listens for Easy Unlock to become disabled. This signals
   * that the turnoff process has succeeded. Regardless of whether the turnoff
   * was initiated from this tab or another, this closes the dialog.
   * @param {boolean} easyUnlockEnabled
   * @private
   */
  handleEasyUnlockEnabledStatusChanged_: function(easyUnlockEnabled) {
    const dialog = /** @type {!CrDialogElement} */ (this.$.dialog);
    if (!easyUnlockEnabled && dialog.open)
      this.onCancelTap_();
  },

  /** @private */
  onCancelTap_: function() {
    this.browserProxy_.cancelTurnOffFlow();
    this.$.dialog.close();
  },

  /** @private */
  onTurnOffTap_: function() {
    this.browserProxy_.startTurnOffFlow();
  },

  /**
   * @param {!settings.EasyUnlockTurnOffStatus} status
   * @return {string}
   * @private
   */
  getTitleText_: function(status) {
    switch (status) {
      case settings.EasyUnlockTurnOffStatus.OFFLINE:
        return this.i18n('easyUnlockTurnOffOfflineTitle');
      case settings.EasyUnlockTurnOffStatus.UNKNOWN:
      case settings.EasyUnlockTurnOffStatus.IDLE:
      case settings.EasyUnlockTurnOffStatus.PENDING:
        return this.i18n('easyUnlockTurnOffTitle');
      case settings.EasyUnlockTurnOffStatus.SERVER_ERROR:
        return this.i18n('easyUnlockTurnOffErrorTitle');
    }
    assertNotReached();
  },

  /**
   * @param {!settings.EasyUnlockTurnOffStatus} status
   * @return {string}
   * @private
   */
  getDescriptionText_: function(status) {
    switch (status) {
      case settings.EasyUnlockTurnOffStatus.OFFLINE:
        return this.i18n('easyUnlockTurnOffOfflineMessage');
      case settings.EasyUnlockTurnOffStatus.UNKNOWN:
      case settings.EasyUnlockTurnOffStatus.IDLE:
      case settings.EasyUnlockTurnOffStatus.PENDING:
        return this.i18n('easyUnlockTurnOffDescription');
      case settings.EasyUnlockTurnOffStatus.SERVER_ERROR:
        return this.i18n('easyUnlockTurnOffErrorMessage');
    }
    assertNotReached();
  },

  /**
   * @param {!settings.EasyUnlockTurnOffStatus} status
   * @return {string}
   * @private
   */
  getTurnOffButtonText_: function(status) {
    switch (status) {
      case settings.EasyUnlockTurnOffStatus.OFFLINE:
        return '';
      case settings.EasyUnlockTurnOffStatus.UNKNOWN:
      case settings.EasyUnlockTurnOffStatus.IDLE:
      case settings.EasyUnlockTurnOffStatus.PENDING:
        return this.i18n('easyUnlockTurnOffButton');
      case settings.EasyUnlockTurnOffStatus.SERVER_ERROR:
        return this.i18n('retry');
    }
    assertNotReached();
  },

  /**
   * @param {!settings.EasyUnlockTurnOffStatus} status
   * @return {boolean}
   * @private
   */
  isButtonBarHidden_: function(status) {
    return status == settings.EasyUnlockTurnOffStatus.OFFLINE;
  },

  /**
   * @param {!settings.EasyUnlockTurnOffStatus} status
   * @return {boolean}
   * @private
   */
  isSpinnerActive_: function(status) {
    return status == settings.EasyUnlockTurnOffStatus.PENDING;
  },

  /**
   * @param {!settings.EasyUnlockTurnOffStatus} status
   * @return {boolean}
   * @private
   */
  isCancelButtonHidden_: function(status) {
    return status == settings.EasyUnlockTurnOffStatus.SERVER_ERROR;
  },

  /**
   * @param {!settings.EasyUnlockTurnOffStatus} status
   * @return {boolean}
   * @private
   */
  isTurnOffButtonEnabled_: function(status) {
    return status == settings.EasyUnlockTurnOffStatus.IDLE ||
        status == settings.EasyUnlockTurnOffStatus.SERVER_ERROR;
  },
});

})();
