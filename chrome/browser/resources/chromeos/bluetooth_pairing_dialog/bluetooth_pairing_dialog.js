// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'bluetooth-dialog-host' is used to host a <bluetooth-dialog> element to
 * manage bluetooth pairing. The device properties are provided in the
 * chrome 'dialogArguments' variable. When created (attached) the dialog
 * connects to the specified device and passes the results to the
 * bluetooth-dialog element to display.
 */

Polymer({
  is: 'bluetooth-pairing-dialog',

  behaviors: [I18nBehavior],

  properties: {
    /**
     * Current Pairing device.
     * @type {!chrome.bluetooth.Device|undefined}
     * @private
     */
    pairingDevice_: Object,
  },

  /** @override */
  attached: function() {
    var dialogArgs = chrome.getVariableValue('dialogArguments');
    this.pairingDevice_ =
        /** @type {!chrome.bluetooth.Device} */ (
            dialogArgs ? JSON.parse(dialogArgs) : {});
    this.connect_();
  },

  /** @private */
  connect_: function() {
    this.$.deviceDialog.open();
    var device = this.pairingDevice_;
    chrome.bluetoothPrivate.connect(device.address, result => {
      var dialog = this.$.deviceDialog;
      dialog.handleError(device, chrome.runtime.lastError, result);
    });
  },

  /** @private */
  onDialogClose_: function() {
    chrome.send('dialogClose');
  },
});
