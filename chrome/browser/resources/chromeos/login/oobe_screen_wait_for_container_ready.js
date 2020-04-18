// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Oobe Wait For Container Ready screen implementation.
 */

login.createScreen(
    'WaitForContainerReadyScreen', 'wait-for-container-ready', function() {
      return {

        /** @Override */
        onBeforeShow: function(data) {
          Oobe.getInstance().headerHidden = true;
        }
      };
    });
