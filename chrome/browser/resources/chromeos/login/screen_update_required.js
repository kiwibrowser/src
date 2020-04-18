// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview "Update is required to sign in" screen.
 */

login.createScreen('UpdateRequiredScreen', 'update-required', function() {
  return {
    /** @Override */
    onBeforeShow: function(data) {
      Oobe.getInstance().headerHidden = true;
    }
  };
});
