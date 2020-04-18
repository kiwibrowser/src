// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @typedef {{
 *   tetherHostDeviceName: string,
 *   batteryPercentage: number,
 *   connectionStrength: number,
 *   isTetherHostCurrentlyOnWifi: boolean
 * }}
 */
let TetherConnectionData;

Polymer({
  is: 'tether-connection-dialog',

  behaviors: [I18nBehavior],

  properties: {
    /**
     * The current properties for the network matching |guid|.
     * @type {!CrOnc.NetworkProperties|undefined}
     */
    networkProperties: {
      type: Object,
    },

    /**
     * Whether the network has been lost (e.g., has gone out of range).
     * @type {boolean}
     */
    outOfRange: Boolean,
  },

  open: function() {
    const dialog = this.getDialog_();
    if (!dialog.open)
      this.getDialog_().showModal();

    this.$.connectButton.focus();
  },

  close: function() {
    const dialog = this.getDialog_();
    if (dialog.open)
      dialog.close();
  },

  /**
   * @return {!CrDialogElement}
   * @private
   */
  getDialog_: function() {
    return /** @type {!CrDialogElement} */ (this.$.dialog);
  },

  /** @private */
  onNotNowTap_: function() {
    this.getDialog_().cancel();
  },

  /**
   * Fires the 'connect-tap' event.
   * @private
   */
  onConnectTap_: function() {
    this.fire('tether-connect');
  },

  /**
   * @param {!CrOnc.NetworkProperties} networkProperties The network
   *     properties.
   * @return {boolean}
   * @private
   */
  shouldShowDisconnectFromWifi_: function(networkProperties) {
    // TODO(khorimoto): Pipe through a new network property which describes
    // whether the tether host is currently connected to a Wi-Fi network. Return
    // whether it is here.
    return true;
  },

  /**
   * @param {!CrOnc.NetworkProperties} networkProperties The network properties.
   * @return {string} The battery percentage integer value converted to a
   *     string. Note that this will not return a string with a "%" suffix.
   * @private
   */
  getBatteryPercentageAsString_: function(networkProperties) {
    const percentage = this.get('Tether.BatteryPercentage', networkProperties);
    if (percentage === undefined)
      return '';
    return percentage.toString();
  },

  /**
   * Retrieves an image that corresponds to signal strength of the tether host.
   * Custom icons are used here instead of a <cr-network-icon> because this
   * dialog uses a special color scheme.
   *
   * @param {!CrOnc.NetworkProperties} networkProperties The network properties.
   * @return {string} The name of the icon to be used to represent the network's
   * signal strength.
   */
  getSignalStrengthIconName_: function(networkProperties) {
    let signalStrength = this.get('Tether.SignalStrength', networkProperties);
    if (signalStrength === undefined)
      signalStrength = 4;
    return 'settings:signal-cellular-' +
        Math.min(4, Math.max(signalStrength, 0)) + '-bar';
  },

  /**
   * @param {!CrOnc.NetworkProperties} networkProperties The network properties.
   * @return {string}
   * @private
   */
  getDeviceName_: function(networkProperties) {
    return CrOnc.getNetworkName(networkProperties);
  },

  /**
   * @param {!CrOnc.NetworkProperties} networkProperties The network properties.
   * @return {string}
   * @private
   */
  getBatteryPercentageString_: function(networkProperties) {
    return this.i18n(
        'tetherConnectionBatteryPercentage',
        this.getBatteryPercentageAsString_(networkProperties));
  },

  /**
   * @param {!CrOnc.NetworkProperties} networkProperties The network properties.
   * @return {string}
   * @private
   */
  getExplanation_: function(networkProperties) {
    return this.i18n(
        'tetherConnectionExplanation',
        CrOnc.getEscapedNetworkName(networkProperties));
  },

  /**
   * @param {!CrOnc.NetworkProperties} networkProperties The network properties.
   * @return {string}
   * @private
   */
  getDescriptionTitle_: function(networkProperties) {
    return this.i18n(
        'tetherConnectionDescriptionTitle',
        CrOnc.getEscapedNetworkName(networkProperties));
  },

  /**
   * @param {!CrOnc.NetworkProperties} networkProperties The network properties.
   * @return {string}
   * @private
   */
  getBatteryDescription_: function(networkProperties) {
    return this.i18n(
        'tetherConnectionDescriptionBattery',
        this.getBatteryPercentageAsString_(networkProperties));
  },
});
