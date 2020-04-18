// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'usb-devices' is the polymer element for showing the USB Devices category
 * under Site Settings.
 */

Polymer({
  is: 'usb-devices',

  behaviors: [SiteSettingsBehavior],

  properties: {
    /**
     * A list of all USB devices.
     * @private {!Array<!UsbDeviceEntry>}
     */
    devices_: Array,

    /**
     * The targeted object for menu operations.
     * @private {?Object}
     */
    actionMenuModel_: Object
  },

  /** @override */
  ready: function() {
    this.fetchUsbDevices_();
  },

  /**
   * Fetch the list of USB devices and update the list.
   * @private
   */
  fetchUsbDevices_: function() {
    this.browserProxy.fetchUsbDevices().then(deviceList => {
      this.devices_ = deviceList;
    });
  },

  /**
   * @return {boolean}
   * @private
   */
  hasDevices_: function() {
    return !!(this.devices_ && this.devices_.length);
  },

  /**
   * A handler when an action is selected in the action menu.
   * @private
   */
  onRemoveTap_: function() {
    this.$$('cr-action-menu').close();

    const item = this.actionMenuModel_;
    this.browserProxy.removeUsbDevice(
        item.origin, item.embeddingOrigin, item.object);
    this.actionMenuModel_ = null;
    this.fetchUsbDevices_();
  },

  /**
   * A handler to show the action menu next to the clicked menu button.
   * @param {!{model: !{item: UsbDeviceEntry}}} event
   * @private
   */
  showMenu_: function(event) {
    this.actionMenuModel_ = event.model.item;
    /** @type {!CrActionMenuElement} */ (this.$$('cr-action-menu'))
        .showAt(
            /** @type {!Element} */ (
                Polymer.dom(/** @type {!Event} */ (event)).localTarget));
  }
});
