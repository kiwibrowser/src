// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'start-setup-page',

  properties: {
    /** Overridden from UiPageContainerBehavior. */
    forwardButtonTextId: {
      type: String,
      value: 'accept',
    },

    /** Overridden from UiPageContainerBehavior. */
    backwardButtonTextId: {
      type: String,
      value: 'cancel',
    },

    /** Overridden from UiPageContainerBehavior. */
    headerId: {
      type: String,
      value: 'startSetupPageHeader',
    },

    /** Overridden from UiPageContainerBehavior. */
    messageId: {
      type: String,
      value: 'startSetupPageMessage',
    },

    /**
     * Array of objects representing all available MultiDevice hosts. Each
     * object contains the name of the device type (e.g. "Pixel XL") and its
     * public key.
     *
     * @type {!Array<{name: string, publicKey: string}>}
     */
    devices: {
      type: Array,
      value: () => [],
      observer: 'devicesChanged_',
    },

    /**
     * Public key for the currently selected host device.
     *
     * Undefined if the no list of potential hosts has been received from mojo
     * service.
     *
     * @type {string|undefined}
     */
    selectedPublicKey: {
      type: String,
      notify: true,
    },
  },

  behaviors: [
    UiPageContainerBehavior,
    I18nBehavior,
  ],

  /**
   * @param {!Array<{name: string, publicKey: string}>} devices Device list.
   * @return {string} Label for devices selection content.
   * @private
   */
  getDeviceSelectionHeader_(devices) {
    switch (devices.length) {
      case 0:
        return '';
      case 1:
        return this.i18n('startSetupPageSingleDeviceHeader');
      default:
        return this.i18n('startSetupPageMultipleDeviceHeader');
    }
  },

  /**
   * @param {!Array<{name: string, publicKey: string}>} devices Device list.
   * @return {boolean} True if there are more than one potential host devices.
   * @private
   */
  doesDeviceListHaveMultipleElements_: function(devices) {
    return devices.length > 1;
  },

  /**
   * @param {!Array<{name: string, publicKey: string}>} devices Device list.
   * @return {boolean} True if there is exactly one potential host device.
   * @private
   */
  doesDeviceListHaveOneElement_: function(devices) {
    return devices.length == 1;
  },

  /**
   * @param {!Array<{name: string, publicKey: string}>} devices Device list.
   * @return {string} Name of the first device in device list if there are any.
   *     Returns an empty string otherwise.
   * @private
   */
  getFirstDeviceNameInList_: function(devices) {
    return devices[0] ? this.devices[0].name : '';
  },

  /** @private */
  devicesChanged_: function() {
    if (this.devices.length > 0)
      this.selectedPublicKey = this.devices[0].publicKey;
  },

  /** @private */
  onDeviceDropdownSelectionChanged_: function() {
    this.selectedPublicKey = this.$.deviceDropdown.value;
  },
});
