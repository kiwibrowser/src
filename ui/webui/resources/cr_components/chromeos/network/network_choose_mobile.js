// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Polymer element for displaying and modifying a list of cellular
 * mobile networks.
 */
Polymer({
  is: 'network-choose-mobile',

  behaviors: [I18nBehavior],

  properties: {
    /**
     * The current set of properties for the network.
     * @type {!CrOnc.NetworkProperties|undefined}
     */
    networkProperties: {
      type: Object,
      observer: 'networkPropertiesChanged_',
    },

    /**
     * Interface for networkingPrivate calls.
     * @type {NetworkingPrivate}
     */
    networkingPrivate: {
      type: Object,
      value: chrome.networkingPrivate,
    },

    /**
     * The networkingPrivate.FoundNetworkProperties.NetworkId of the selected
     * mobile network.
     * @private
     */
    selectedMobileNetworkId_: {
      type: String,
      value: '',
    },

    /**
     * Selectable list of FoundNetwork dictionaries for the UI.
     * @private {!Array<!chrome.networkingPrivate.FoundNetworkProperties>}
     */
    mobileNetworkList_: {
      type: Array,
      value: function() {
        return [];
      }
    },
  },

  /** @private {boolean} */
  scanRequested_: false,

  /** @override */
  attached: function() {
    this.scanRequested_ = false;
  },

  /**
   * Polymer networkProperties changed method.
   */
  networkPropertiesChanged_: function() {
    if (!this.networkProperties || !this.networkProperties.Cellular)
      return;
    var cellular = this.networkProperties.Cellular;
    this.mobileNetworkList_ = cellular.FoundNetworks ||
        [{NetworkId: 'none', LongName: this.i18n('networkCellularNoNetworks')}];

    // Set selectedMobileNetworkId_ after the dom-repeat has been stamped.
    this.async(() => {
      var selected = this.mobileNetworkList_.find(function(mobileNetwork) {
        return mobileNetwork.Status == 'current';
      });
      if (!selected)
        selected = this.mobileNetworkList_[0];
      this.selectedMobileNetworkId_ = selected.NetworkId;
    });
  },

  /**
   * @param {!chrome.networkingPrivate.FoundNetworkProperties} network
   * @return {boolean}
   * @private
   */
  getMobileNetworkIsDisabled_: function(network) {
    return network.Status != 'available' && network.Status != 'current';
  },

  /**
   * @param {!chrome.networkingPrivate.NetworkProperties} properties
   * @return {boolean}
   * @private
   */
  getEnableScanButton_: function(properties) {
    return properties.ConnectionState == CrOnc.ConnectionState.NOT_CONNECTED &&
        !this.get('Cellular.Scanning', properties);
  },

  /**
   * @param {!chrome.networkingPrivate.NetworkProperties} properties
   * @return {boolean}
   * @private
   */
  getEnableSelectNetwork_: function(properties) {
    if (this.get('Cellular.Scanning', properties) ||
        properties.ConnectionState != CrOnc.ConnectionState.NOT_CONNECTED) {
      return false;
    }
    var found = this.get('Cellular.FoundNetworks', properties);
    return !!found && found.length > 0;
  },

  /**
   * @param {!chrome.networkingPrivate.NetworkProperties} properties
   * @return {string}
   * @private
   */
  getSecondaryText_: function(properties) {
    if (!properties || !properties.Cellular)
      return '';
    var cellular = properties.Cellular;
    if (cellular.Scanning)
      return this.i18n('networkCellularScanning');
    else if (this.scanRequested_)
      return this.i18n('networkCellularScanCompleted');
    else if (properties.ConnectionState != CrOnc.ConnectionState.NOT_CONNECTED)
      return this.i18n('networkCellularScanConnectedHelp');
    return '';
  },

  /**
   * @param {!chrome.networkingPrivate.FoundNetworkProperties} network
   * @return {string}
   * @private
   */
  getName_: function(network) {
    return network.LongName || network.ShortName || network.NetworkId;
  },

  /**
   * Request a Cellular scan to populate the list of networks. This will
   * triger a change to networkProperties when completed (if
   * Cellular.FoundNetworks changes).
   * @private
   */
  onScanTap_: function() {
    this.scanRequested_ = true;
    this.networkingPrivate.requestNetworkScan(CrOnc.Type.CELLULAR);
  },

  /**
   * @param {!Event} event
   * @private
   */
  onChange_: function(event) {
    var target = /** @type {!HTMLSelectElement} */ (event.target);
    if (!target.value || target.value == 'none')
      return;
    this.networkingPrivate.selectCellularMobileNetwork(
        this.networkProperties.GUID, target.value);
  },
});
