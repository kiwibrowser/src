// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Polymer element wrapping cr-network-list including the
 * networkingPrivate calls to populate it.
 */

Polymer({
  is: 'cr-network-select',

  properties: {
    /**
     * Show all buttons in list items.
     */
    showButtons: {
      type: Boolean,
      value: false,
      reflectToAttribute: true,
    },

    /**
     * The list of custom items to display after the list of networks.
     * See CrNetworkList for details.
     * @type {!Array<CrNetworkList.CustomItemState>}
     */
    customItems: {
      type: Array,
      value: function() {
        return [];
      },
    },

    /**
     * Whether to handle "item-selected" for network items.
     * If this property is false, "network-item-selected" event is fired
     * carrying CrOnc.NetworkStateProperties as event detail.
     * @type {Function}
     */
    handleNetworkItemSelected: {
      type: Boolean,
      value: false,
      reflectToAttribute: true,
    },

    /**
     * List of all network state data for all visible networks.
     * @private {!Array<!CrOnc.NetworkStateProperties>}
     */
    networkStateList_: {
      type: Array,
      value: function() {
        return [];
      },
    },

    /**
     * Cached Cellular Device state or undefined if there is no Cellular device.
     * @private {!CrOnc.DeviceStateProperties|undefined} deviceState
     */
    cellularDeviceState_: Object,
  },

  /** @type {!CrOnc.NetworkStateProperties|undefined} */
  defaultNetworkState_: undefined,

  focus: function() {
    this.$.networkList.focus();
  },

  /**
   * Listener function for chrome.networkingPrivate.onNetworkListChanged event.
   * @type {function(!Array<string>)}
   * @private
   */
  networkListChangedListener_: function() {},

  /**
   * Listener function for chrome.networkingPrivate.onDeviceStateListChanged
   * event.
   * @type {function(!Array<string>)}
   * @private
   */
  deviceStateListChangedListener_: function() {},

  /** @private {number|null} */
  scanIntervalId_: null,

  /** @override */
  attached: function() {
    this.networkListChangedListener_ = this.refreshNetworks.bind(this);
    chrome.networkingPrivate.onNetworkListChanged.addListener(
        this.networkListChangedListener_);

    this.deviceStateListChangedListener_ = this.refreshNetworks.bind(this);
    chrome.networkingPrivate.onDeviceStateListChanged.addListener(
        this.deviceStateListChangedListener_);

    this.refreshNetworks();

    /** @const */ var INTERVAL_MS = 10 * 1000;
    chrome.networkingPrivate.requestNetworkScan();
    this.scanIntervalId_ = window.setInterval(function() {
      chrome.networkingPrivate.requestNetworkScan();
    }.bind(this), INTERVAL_MS);
  },

  /** @override */
  detached: function() {
    if (this.scanIntervalId_ !== null)
      window.clearInterval(this.scanIntervalId_);
    chrome.networkingPrivate.onNetworkListChanged.removeListener(
        this.networkListChangedListener_);
    chrome.networkingPrivate.onDeviceStateListChanged.removeListener(
        this.deviceStateListChangedListener_);
  },

  /**
   * Requests the device and network states. May be called externally to force a
   * refresh and list update (e.g. when the element is shown).
   */
  refreshNetworks: function() {
    chrome.networkingPrivate.getDeviceStates(
        this.getDeviceStatesCallback_.bind(this));
  },

  /**
   * @param {!Array<!CrOnc.DeviceStateProperties>} deviceStates
   * @private
   */
  getDeviceStatesCallback_: function(deviceStates) {
    var filter = {
      networkType: chrome.networkingPrivate.NetworkType.ALL,
      visible: true,
      configured: false
    };
    chrome.networkingPrivate.getNetworks(filter, function(networkStates) {
      this.getNetworksCallback_(deviceStates, networkStates);
    }.bind(this));
  },

  /**
   * @param {!Array<!CrOnc.DeviceStateProperties>} deviceStates
   * @param {!Array<!CrOnc.NetworkStateProperties>} networkStates
   * @private
   */
  getNetworksCallback_: function(deviceStates, networkStates) {
    this.cellularDeviceState_ = deviceStates.find(function(device) {
      return device.Type == CrOnc.Type.CELLULAR;
    });
    if (this.cellularDeviceState_)
      this.ensureCellularNetwork_(networkStates);
    this.networkStateList_ = networkStates;
    var defaultNetwork;
    for (var i = 0; i < networkStates.length; ++i) {
      var state = networkStates[i];
      if (state.ConnectionState == CrOnc.ConnectionState.CONNECTED) {
        defaultNetwork = state;
        break;
      }
      if (state.ConnectionState == CrOnc.ConnectionState.CONNECTING &&
          !defaultNetwork) {
        defaultNetwork = state;
        // Do not break here in case a non WiFi network is connecting but a
        // WiFi network is connected.
      } else if (state.Type == CrOnc.Type.WI_FI) {
        break;  // Non connecting or connected WiFI networks are always last.
      }
    }
    if ((!defaultNetwork && !this.defaultNetworkState_) ||
        (defaultNetwork && this.defaultNetworkState_ &&
         defaultNetwork.GUID == this.defaultNetworkState_.GUID &&
         defaultNetwork.ConnectionState ==
             this.defaultNetworkState_.ConnectionState)) {
      return;  // No change to network or ConnectionState
    }
    this.defaultNetworkState_ = defaultNetwork ?
        /** @type {!CrOnc.NetworkStateProperties|undefined} */ (
            Object.assign({}, defaultNetwork)) :
        undefined;
    this.fire('default-network-changed', defaultNetwork);
  },

  /**
   * Modifies |networkStates| to include a cellular network if none exists.
   * @param {!Array<!CrOnc.NetworkStateProperties>} networkStates
   * @private
   */
  ensureCellularNetwork_: function(networkStates) {
    if (networkStates.find(function(network) {
          return network.Type == CrOnc.Type.CELLULAR;
        })) {
      return;
    }
    // Add a Cellular network after the Ethernet network if it exists.
    var idx = networkStates.length > 0 &&
            networkStates[0].Type == CrOnc.Type.ETHERNET ?
        1 :
        0;
    var cellular = {
      GUID: '',
      Type: CrOnc.Type.CELLULAR,
      Cellular: {Scanning: this.cellularDeviceState_.Scanning}
    };
    networkStates.splice(idx, 0, cellular);
  },

  /**
   * Event triggered when a cr-network-list-item is selected.
   * @param {!{target: HTMLElement, detail: !CrOnc.NetworkStateProperties}} e
   * @private
   */
  onNetworkListItemSelected_: function(e) {
    var state = e.detail;
    e.target.blur();

    if (!this.handleNetworkItemSelected) {
      this.fire('network-item-selected', state);
      return;
    }

    // NOTE: This isn't used by OOBE (no handle-network-item-selected).
    // TODO(stevenjb): Remove custom OOBE handling.
    if (state.Type == CrOnc.Type.CELLULAR && this.cellularDeviceState_) {
      var cellularDevice = this.cellularDeviceState_;
      // If Cellular is not enabled and not SIM locked, enable Cellular.
      if (cellularDevice.State != CrOnc.DeviceState.ENABLED &&
          (!cellularDevice.SIMLockStatus ||
           !cellularDevice.SIMLockStatus.LockType)) {
        chrome.networkingPrivate.enableNetworkType(CrOnc.Type.CELLULAR);
      }
    }

    if (state.ConnectionState != CrOnc.ConnectionState.NOT_CONNECTED)
      return;

    chrome.networkingPrivate.startConnect(state.GUID, function() {
      var lastError = chrome.runtime.lastError;
      if (lastError && lastError != 'connecting')
        console.error('networkingPrivate.startConnect error: ' + lastError);
    });
  },
});
