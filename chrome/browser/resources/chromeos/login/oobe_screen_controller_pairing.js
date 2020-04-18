// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview controller pairing screen implementation.
 */

login.createScreen('ControllerPairingScreen', 'controller-pairing', function() {
  return {
    decorate: function() {
      this.children[0].decorate(this);
    }
  };
});
