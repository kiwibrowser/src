// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Utility functions for all tests.
 */
var testUtils = {
  /**
   * Forces failure in tests. Should be called only from 'beforeEach',
   * 'afterEach' and 'it'. Useful to force failures in promises.
   * @param {Object|string} An error with stack trace or a string error that
   *     describes the failure reason.
   */
  forceFailure: function(error) {
    console.error(error.stack || error);
    setTimeout(function() {
      expect(false).to.be.true;
    });
  }
};
