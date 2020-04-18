// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'settings-smb-shares-page',

  behaviors: [WebUIListenerBehavior],

  properties: {
    /** @private */
    showAddSmbDialog_: Boolean,

    /** @private */
    addShareResultText_: String,
  },

  /** @override */
  attached: function() {
    this.addWebUIListener('on-add-smb-share', this.onAddShare_.bind(this));
  },

  /** @private */
  onAddShareTap_: function() {
    this.showAddSmbDialog_ = true;
  },

  /** @private */
  onAddSmbDialogClosed_: function() {
    this.showAddSmbDialog_ = false;
  },

  /**
   * @param {SmbMountResult} result
   * @private
   */
  onAddShare_: function(result) {
    switch (result) {
      case SmbMountResult.SUCCESS:
        this.addShareResultText_ =
            loadTimeData.getString('smbShareAddedSuccessfulMessage');
        break;
      case SmbMountResult.AUTHENTICATION_FAILED:
        this.addShareResultText_ =
            loadTimeData.getString('smbShareAddedAuthFailedMessage');
        break;
      case SmbMountResult.NOT_FOUND:
        this.addShareResultText_ =
            loadTimeData.getString('smbShareAddedNotFoundMessage');
        break;
      case SmbMountResult.UNSUPPORTED_DEVICE:
        this.addShareResultText_ =
            loadTimeData.getString('smbShareAddedUnsupportedDeviceMessage');
        break;
      case SmbMountResult.MOUNT_EXISTS:
        this.addShareResultText_ =
            loadTimeData.getString('smbShareAddedMountExistsMessage');
        break;
      default:
        this.addShareResultText_ =
            loadTimeData.getString('smbShareAddedErrorMessage');
    }
    this.$.errorToast.show();
  },

});
