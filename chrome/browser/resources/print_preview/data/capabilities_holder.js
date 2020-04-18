// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview', function() {
  'use strict';

  /* Mutable reference to a CDD object. */
  class CapabilitiesHolder {
    constructor() {
      /**
       * Reference to the capabilities object.
       * @private {?print_preview.Cdd}
       */
      this.capabilities_ = null;
    }

    /** @return {?print_preview.Cdd} The instance held by the holder. */
    get() {
      return this.capabilities_;
    }

    /**
     * @param {!print_preview.Cdd} capabilities New instance to put into the
     *     holder.
     */
    set(capabilities) {
      this.capabilities_ = capabilities;
    }
  }

  // Export
  return {CapabilitiesHolder: CapabilitiesHolder};
});
