// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview host pairing screen implementation.
 */

login.createScreen('HostPairingScreen', 'host-pairing', function() {
  /**
   * We can't pass Polymer screen directly to login.createScreen, because it
   * changes object's prototype chain.
   */
  return {
    polymerScreen_: null,

    decorate: function() {
      polymerScreen_ = this.children[0];
      polymerScreen_.decorate(this);
    },

    onBeforeShow: function() {
      polymerScreen_.onBeforeShow();
    }
  };
});
