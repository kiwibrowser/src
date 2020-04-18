// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Polymer element for displaying material design Sync Consent
 * screen.
 */

Polymer({
  is: 'sync-consent',

  behaviors: [I18nBehavior],

  /** @override */
  ready: function() {
    this.updateLocalizedContent();
  },

  focus: function() {
    this.$.syncConsentOverviewDialog.focus();
  },

  /**
   * Hides all screens to help switching from one screen to another.
   * @private
   */
  hideAllScreens_: function() {
    var screens = Polymer.dom(this.root).querySelectorAll('oobe-dialog');
    for (let screen of screens)
      screen.hidden = true;
  },

  /**
   * Shows given screen.
   * @param id String Screen ID.
   * @private
   */
  showScreen_: function(id) {
    this.hideAllScreens_();

    var screen = this.$[id];
    assert(screen);
    screen.hidden = false;
    screen.show();
  },

  /**
   * Reacts to changes in loadTimeData.
   */
  updateLocalizedContent: function() {
    let useMakeBetterScreen = loadTimeData.getBoolean('syncConsentMakeBetter');
    if (useMakeBetterScreen) {
      if (this.$.syncConsentMakeChromeSyncOptionsDialog.hidden)
        this.showScreen_('syncConsentNewDialog');
    } else {
      this.showScreen_('syncConsentOverviewDialog');
    }
    this.i18nUpdateLocale();
  },

  /**
   * This is 'on-tap' event handler for 'AcceptAndContinue' button.
   * @private
   */
  onSettingsSaveAndContinue_: function() {
    if (this.$.reviewSettingsBox.checked) {
      chrome.send('login.SyncConsentScreen.userActed', ['continue-and-review']);
    } else {
      chrome.send(
          'login.SyncConsentScreen.userActed', ['continue-with-defaults']);
    }
  },

  /******************************************************
   * Get Google smarts in Chrome dialog.
   ******************************************************/

  /**
   * @private
   */
  onMoreOptionsButton_: function() {
    this.showScreen_('syncConsentMakeChromeSyncOptionsDialog');
  },

  /**
   * @private
   */
  onConfirm_: function() {
    chrome.send(
        'login.SyncConsentScreen.userActed',
        ['continue-with-sync-and-personalization']);
  },

  /******************************************************
   * Get Google smarts ... options dialog
   ******************************************************/

  /** @private */
  onOptionsAcceptAndContinue_: function() {
    const selected = this.$.optionsGroup.selected;
    if (selected == 'justSync') {
      chrome.send(
          'login.SyncConsentScreen.userActed', ['continue-with-sync-only']);
    } else if (selected == 'syncAndPersonalization') {
      chrome.send(
          'login.SyncConsentScreen.userActed',
          ['continue-with-sync-and-personalization']);
    } else {
      // 'Continue and review' is default option.
      chrome.send('login.SyncConsentScreen.userActed', ['continue-and-review']);
    }
  },
});
