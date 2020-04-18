// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * The status view at the top of the page after stopping capturing.
 */
var HaltedStatusView = (function() {
  'use strict';

  // We inherit from DivView.
  var superClass = DivView;

  function HaltedStatusView() {
    superClass.call(this, HaltedStatusView.MAIN_BOX_ID);
  }

  HaltedStatusView.MAIN_BOX_ID = 'halted-status-view';

  HaltedStatusView.prototype = {
    // Inherit the superclass's methods.
    __proto__: superClass.prototype
  };

  return HaltedStatusView;
})();
