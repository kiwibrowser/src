// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'settings-detailed-build-info' contains detailed build
 * information for ChromeOS.
 */

Polymer({
  is: 'settings-detailed-build-info',

  behaviors: [I18nBehavior],

  properties: {
    /** @private {!VersionInfo} */
    versionInfo_: Object,

    /** @private */
    currentlyOnChannelText_: String,

    /** @private */
    showChannelSwitcherDialog_: Boolean,

    /** @private */
    canChangeChannel_: Boolean,
  },

  /** @override */
  ready: function() {
    const browserProxy = settings.AboutPageBrowserProxyImpl.getInstance();
    browserProxy.pageReady();

    browserProxy.getVersionInfo().then(versionInfo => {
      this.versionInfo_ = versionInfo;
    });

    this.updateChannelInfo_();
  },

  /** @private */
  updateChannelInfo_: function() {
    const browserProxy = settings.AboutPageBrowserProxyImpl.getInstance();
    browserProxy.getChannelInfo().then(info => {
      // Display the target channel for the 'Currently on' message.
      this.currentlyOnChannelText_ = this.i18n(
          'aboutCurrentlyOnChannel',
          this.i18n(settings.browserChannelToI18nId(info.targetChannel)));
      this.canChangeChannel_ = info.canChangeChannel;
    });
  },

  /**
   * @param {string} version
   * @return {boolean}
   * @private
   */
  shouldShowVersion_: function(version) {
    return version.length > 0;
  },

  /**
   * @param {boolean} canChangeChannel
   * @return {string}
   * @private
   */
  getChangeChannelIndicatorSourceName_: function(canChangeChannel) {
    return loadTimeData.getBoolean('aboutEnterpriseManaged') ?
        '' :
        loadTimeData.getString('ownerEmail');
  },

  /**
   * @param {boolean} canChangeChannel
   * @return {CrPolicyIndicatorType}
   * @private
   */
  getChangeChannelIndicatorType_: function(canChangeChannel) {
    if (canChangeChannel)
      return CrPolicyIndicatorType.NONE;
    return loadTimeData.getBoolean('aboutEnterpriseManaged') ?
        CrPolicyIndicatorType.DEVICE_POLICY :
        CrPolicyIndicatorType.OWNER;
  },

  /**
   * @param {!Event} e
   * @private
   */
  onChangeChannelTap_: function(e) {
    e.preventDefault();
    this.showChannelSwitcherDialog_ = true;
  },

  /** @private */
  onChannelSwitcherDialogClosed_: function() {
    this.showChannelSwitcherDialog_ = false;
    cr.ui.focusWithoutInk(assert(this.$$('paper-button')));
    this.updateChannelInfo_();
  },
});
