// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'settings-google-assistant-page' is the settings page
 * containing Google Assistant settings.
 */
Polymer({
  is: 'settings-google-assistant-page',

  behaviors: [I18nBehavior, PrefsBehavior],

  /** @private {?settings.GoogleAssistantBrowserProxy} */
  browserProxy_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = settings.GoogleAssistantBrowserProxyImpl.getInstance();
  },

  /**
   * @param {boolean} toggleValue
   * @return {string}
   * @private
   */
  getAssistantOnOffLabel_: function(toggleValue) {
    return this.i18n(toggleValue ? 'toggleOn' : 'toggleOff');
  },

  /** @private */
  onGoogleAssistantEnableChange_: function() {
    this.browserProxy_.setGoogleAssistantEnabled(
        !!this.getPref('settings.voice_interaction.enabled.value'));
  },

  /** @private */
  onGoogleAssistantContextEnableChange_: function() {
    this.browserProxy_.setGoogleAssistantContextEnabled(
        !!this.getPref('settings.voice_interaction.context.enabled.value'));
  },

  /** @private */
  onGoogleAssistantSettingsTapped_: function() {
    this.browserProxy_.showGoogleAssistantSettings();
  },
});
