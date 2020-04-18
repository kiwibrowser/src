// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview A helper object used for Internet page. */
cr.exportPath('settings');

/**
 * @typedef {{
 *   PackageName: string,
 *   ProviderName: string,
 *   AppID: string,
 *   LastLaunchTime: number,
 * }}
 */
settings.ArcVpnProvider;

cr.define('settings', function() {
  /** @interface */
  class InternetPageBrowserProxy {
    /**
     *  Shows configuration of connected external VPN network.
     *  @param {string} guid
     */
    showNetworkConfigure(guid) {}

    /**
     * Sends add VPN request to external VPN provider.
     * @param {string} networkType
     * @param {string} appId
     */
    addThirdPartyVpn(networkType, appId) {}

    /**
     * Requests Chrome to send list of Arc VPN providers.
     */
    requestArcVpnProviders() {}

    /**
     * |callback| is run when there is update of Arc VPN providers.
     * Available after |requestArcVpnProviders| has been called.
     * @param {function(?Array<settings.ArcVpnProvider>):void} callback
     */
    setUpdateArcVpnProvidersCallback(callback) {}

    /**
     * Requests that Chrome send the list of devices whose "Google Play
     * Services" notifications are disabled (these notifications must be enabled
     * to utilize Instant Tethering). The names will be provided via
     * setGmsCoreNotificationsDisabledDeviceNamesCallback().
     */
    requestGmsCoreNotificationsDisabledDeviceNames() {}

    /**
     * Sets the callback to be used to receive the list of devices whose "Google
     * Play Services" notifications are disabled. |callback| is invoked with an
     * array of the names of these devices; note that if no devices have this
     * property, the provided list of device names is empty.
     * @param {function(!Array<string>):void} callback
     */
    setGmsCoreNotificationsDisabledDeviceNamesCallback(callback) {}
  }

  /**
   * @implements {settings.InternetPageBrowserProxy}
   */
  class InternetPageBrowserProxyImpl {
    /** @override */
    showNetworkConfigure(guid) {
      chrome.send('configureNetwork', [guid]);
    }

    /** @override */
    addThirdPartyVpn(networkType, appId) {
      chrome.send('addNetwork', [networkType, appId]);
    }

    /** @override */
    requestArcVpnProviders() {
      chrome.send('requestArcVpnProviders');
    }

    /** @override */
    setUpdateArcVpnProvidersCallback(callback) {
      cr.addWebUIListener('sendArcVpnProviders', callback);
    }

    /** @override */
    requestGmsCoreNotificationsDisabledDeviceNames() {
      chrome.send('requestGmsCoreNotificationsDisabledDeviceNames');
    }

    /** @override */
    setGmsCoreNotificationsDisabledDeviceNamesCallback(callback) {
      cr.addWebUIListener(
          'sendGmsCoreNotificationsDisabledDeviceNames', callback);
    }
  }

  cr.addSingletonGetter(InternetPageBrowserProxyImpl);

  return {
    InternetPageBrowserProxy: InternetPageBrowserProxy,
    InternetPageBrowserProxyImpl: InternetPageBrowserProxyImpl,
  };
});
