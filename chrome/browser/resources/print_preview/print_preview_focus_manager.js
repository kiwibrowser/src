// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview', function() {
  'use strict';

  /**
   * FocusManager implementation specialized for Print Preview, which ensures
   * that Print Preview itself does not receive focus when an overlay is shown.
   * @constructor
   * @extends {cr.ui.FocusManager}
   */
  function PrintPreviewFocusManager() {}

  cr.addSingletonGetter(PrintPreviewFocusManager);

  PrintPreviewFocusManager.prototype = {
    __proto__: cr.ui.FocusManager.prototype,

    /** @override */
    getFocusParent: function() {
      let el = document.body;
      let newEl = null;
      while (newEl = el.querySelector('.overlay:not([hidden])'))
        el = newEl;
      return el;
    }
  };

  // Export
  return {PrintPreviewFocusManager: PrintPreviewFocusManager};
});
