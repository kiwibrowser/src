// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * @suppress {checkTypes}
 * Browser test for the scenario below:
 * 1. Attempt to connect.
 * 2. Hit cancel at the PIN prompt.
 * 3. Reconnect with the PIN.
 * 4. Verify that the session is connected.
 */

'use strict';

/** @constructor */
browserTest.Cancel_PIN = function() {};

/**
 * @param {{pin:string}} data
 */
browserTest.Cancel_PIN.prototype.run = function(data) {
  browserTest.expect(typeof data.pin == 'string');

  var AppMode = remoting.AppMode;
  browserTest.connectMe2Me().then(function() {
    browserTest.clickOnControl('#pin-dialog .cancel-button');
    return browserTest.onUIMode(AppMode.HOME);
  }).then(function() {
    return browserTest.connectMe2Me()
  }).then(function() {
    return browserTest.enterPIN(data.pin)
  }).then(function() {
    // On fulfilled.
    browserTest.pass();
  }, function(reason) {
    // On rejected.
    browserTest.fail(reason);
  });
};
