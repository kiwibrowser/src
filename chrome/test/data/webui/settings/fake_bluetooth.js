// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Fake implementation of chrome.bluetooth for testing.
 */
cr.define('settings', function() {
  /**
   * Fake of the chrome.bluetooth API.
   * @constructor
   * @implements {Bluetooth}
   */
  function FakeBluetooth() {
    /** @type {!chrome.bluetooth.AdapterState} */ this.adapterState_ = {
      address: '00:11:22:33:44:55:66',
      name: 'Fake Adapter',
      powered: false,
      available: true,
      discovering: false
    };

    /** @type {!Array<!chrome.bluetooth.Device>} */ this.devices = [];
  }

  FakeBluetooth.prototype = {
    // Public testing methods.
    /** @param {boolean} enabled */
    setEnabled: function(enabled) {
      this.setAdapterState({powered: enabled});
    },

    /** @param {!chrome.bluetooth.AdapterState} state*/
    setAdapterState: function(state) {
      Object.assign(this.adapterState_, state);
      this.onAdapterStateChanged.callListeners(
          Object.assign({}, this.adapterState_));
    },

    /** @return {!chrome.bluetooth.AdapterState} */
    getAdapterStateForTest: function() {
      return Object.assign({}, this.adapterState_);
    },

    /** @param {!Array<!chrome.bluetooth.Device>} devices */
    setDevicesForTest: function(devices) {
      for (const d of this.devices)
        this.onDeviceRemoved.callListeners(d);
      this.devices = devices.slice();
      for (const d of this.devices)
        this.onDeviceAdded.callListeners(d);
    },

    /**
     * @param {string}
     * @return {!chrome.bluetooth.Device}
     */
    getDeviceForTest: function(address) {
      return this.devices.find(function(d) {
        return d.address == address;
      });
    },

    /** @param {!chrome.bluetooth.Device} device */
    updateDeviceForTest: function(device, opt_callback) {
      const index = this.devices.findIndex(function(d) {
        return d.address == device.address;
      });
      if (index == -1) {
        this.devices.push(device);
        this.onDeviceAdded.callListeners(device);
        return;
      }
      this.devices[index] = device;
      this.onDeviceChanged.callListeners(device);
    },

    // Bluetooth overrides.
    /** @override */
    getAdapterState: function(callback) {
      callback(Object.assign({}, this.adapterState_));
    },

    /** @override */
    getDevice: assertNotReached,

    /** @override */
    getDevices: function(opt_filter, opt_callback) {
      if (opt_callback)
        opt_callback(this.devices.slice());
    },

    /** @override */
    startDiscovery: function(callback) {
      callback();
    },

    /** @override */
    stopDiscovery: assertNotReached,

    /** @override */
    onAdapterStateChanged: new FakeChromeEvent(),

    /** @override */
    onDeviceAdded: new FakeChromeEvent(),

    /** @override */
    onDeviceChanged: new FakeChromeEvent(),

    /** @override */
    onDeviceRemoved: new FakeChromeEvent(),
  };

  return {FakeBluetooth: FakeBluetooth};
});
