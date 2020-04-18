// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'settings-powerwash-dialog' is a dialog shown to request confirmation from
 * the user for a device reset (aka powerwash).
 */
Polymer({
  is: 'settings-powerwash-dialog',

  properties: {
    /** @public */
    requestTpmFirmwareUpdate: {
      type: Boolean,
      value: false,
    }
  },

  /** @override */
  attached: function() {
    settings.ResetBrowserProxyImpl.getInstance().onPowerwashDialogShow();
    this.$.dialog.showModal();
  },

  /** @private */
  onCancelTap_: function() {
    this.$.dialog.close();
  },

  /** @private */
  onRestartTap_: function() {
    settings.LifetimeBrowserProxyImpl.getInstance().factoryReset(
        this.requestTpmFirmwareUpdate);
  },
});
