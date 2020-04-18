// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * @suppress {checkTypes}
 * Browser test for the scenario below:
 * 1. Attempt to connect.
 * 2. Enter |data.pin| at the PIN prompt.
 * 3. Verify that there is connection error due to invalid access code.
 */

'use strict';

/** @constructor */
browserTest.Invalid_PIN = function() {};

/**
 * @param {{pin:string}} data
 */
browserTest.Invalid_PIN.prototype.run = function(data) {
  // Input validation.
  browserTest.expect(typeof data.pin == 'string');

  // Connect to me2me Host.
  browserTest.connectMe2Me().then(function(){
    return browserTest.enterPIN(data.pin, true)
  }).then(
    // Sleep for two seconds to allow the host backoff timer to reset.
    base.Promise.sleep.bind(window, 2000)
  ).then(function() {
    // On fulfilled.
    browserTest.pass();
  }, function(reason) {
    // On rejected.
    browserTest.fail(reason);
  });
};
