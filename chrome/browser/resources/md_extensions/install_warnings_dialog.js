// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('extensions', function() {
  'use strict';

  const InstallWarningsDialog = Polymer({
    is: 'extensions-install-warnings-dialog',

    properties: {
      /** @type {!Array<string>} */
      installWarnings: Array,
    },

    /** @override */
    attached: function() {
      this.$.dialog.showModal();
    },

    /** @private */
    onOkTap_: function() {
      this.$.dialog.close();
    },
  });

  return {InstallWarningsDialog: InstallWarningsDialog};
});
