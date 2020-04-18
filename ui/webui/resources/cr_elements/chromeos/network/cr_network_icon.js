// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Polymer element for rendering network icons based on ONC
 * state properties.
 */

Polymer({
  is: 'cr-network-icon',

  properties: {
    /**
     * If set, the ONC properties will be used to display the icon. This may
     * either be the complete set of NetworkProperties or the subset of
     * NetworkStateProperties.
     * @type {!CrOnc.NetworkProperties|!CrOnc.NetworkStateProperties|undefined}
     */
    networkState: Object,

    /**
     * If set, the device state for the network type.
     * @type {!CrOnc.DeviceStateProperties|undefined}
     */
    deviceState: Object,

    /**
     * If true, the icon is part of a list of networks and may be displayed
     * differently, e.g. the disconnected image will never be shown for
     * list items.
     */
    isListItem: {
      type: Boolean,
      value: false,
    },
  },

  /**
   * @return {string} The name of the svg icon image to show.
   * @private
   */
  getIconClass_: function() {
    if (!this.networkState)
      return '';
    var type = this.networkState.Type;
    if (type == CrOnc.Type.ETHERNET)
      return 'ethernet';
    if (type == CrOnc.Type.VPN)
      return 'vpn';

    var prefix = (type == CrOnc.Type.CELLULAR || type == CrOnc.Type.TETHER) ?
        'cellular-' :
        'wifi-';
    if (!this.isListItem && !this.networkState.GUID) {
      var deviceState = this.deviceState;
      if (!deviceState || deviceState.State == 'Enabled' ||
          deviceState.State == 'Enabling') {
        return prefix + 'no-network';
      }
      return prefix + 'off';
    }

    var connectionState = this.networkState.ConnectionState;
    if (connectionState == CrOnc.ConnectionState.CONNECTING)
      return prefix + 'connecting';

    if (!this.isListItem &&
        (!connectionState ||
         connectionState == CrOnc.ConnectionState.NOT_CONNECTED)) {
      return prefix + 'not-connected';
    }

    var strength = CrOnc.getSignalStrength(this.networkState);
    return prefix + this.strengthToIndex_(strength).toString(10);
  },

  /**
   * @param {number} strength The signal strength from [0 - 100].
   * @return {number} An index from 0-4 corresponding to |strength|.
   * @private
   */
  strengthToIndex_: function(strength) {
    if (strength == 0)
      return 0;
    return Math.min(Math.trunc((strength - 1) / 25) + 1, 4);
  },

  /**
   * @return {boolean}
   * @private
   */
  showTechnology_: function() {
    return this.getTechnology_() != '';
  },

  /**
   * @return {string}
   * @private
   */
  getTechnology_: function() {
    var networkState = this.networkState;
    if (!networkState)
      return '';
    var type = networkState.Type;
    if (type == CrOnc.Type.WI_MAX)
      return 'network:4g';
    if (type == CrOnc.Type.CELLULAR && networkState.Cellular) {
      var technology =
          this.getTechnologyId_(networkState.Cellular.NetworkTechnology);
      if (technology != '')
        return 'network:' + technology;
    }
    return '';
  },

  /**
   * @param {string|undefined} networkTechnology
   * @return {string}
   * @private
   */
  getTechnologyId_: function(networkTechnology) {
    switch (networkTechnology) {
      case CrOnc.NetworkTechnology.CDMA1XRTT:
        return 'badge-1x';
      case CrOnc.NetworkTechnology.EDGE:
        return 'badge-edge';
      case CrOnc.NetworkTechnology.EVDO:
        return 'badge-evdo';
      case CrOnc.NetworkTechnology.GPRS:
      case CrOnc.NetworkTechnology.GSM:
        return 'badge-gsm';
      case CrOnc.NetworkTechnology.HSPA:
        return 'badge-hspa';
      case CrOnc.NetworkTechnology.HSPA_PLUS:
        return 'badge-hspa-plus';
      case CrOnc.NetworkTechnology.LTE:
        return 'badge-lte';
      case CrOnc.NetworkTechnology.LTE_ADVANCED:
        return 'badge-lte-advanced';
      case CrOnc.NetworkTechnology.UMTS:
        return 'badge-3g';
    }
    return '';
  },

  /**
   * @return {boolean}
   * @private
   */
  showSecure_: function() {
    var networkState = this.networkState;
    if (!this.networkState)
      return false;
    if (networkState.Type != CrOnc.Type.WI_FI || !networkState.WiFi)
      return false;
    if (!this.isListItem &&
        networkState.ConnectionState == CrOnc.ConnectionState.NOT_CONNECTED) {
      return false;
    }
    var security = CrOnc.getStateOrActiveString(networkState.WiFi.Security);
    return !!security && security != 'None';
  },
});
