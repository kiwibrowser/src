// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * @suppress {checkTypes}
 * Browser test for the scenario below:
 * 1. Change the PIN.
 * 2. Connect with the new PIN.
 * 3. Verify the connection succeeded.
 * 4. Disconnect and reconnect with the old PIN.
 * 5. Verify the connection failed.
 */

'use strict';

/** @constructor */
browserTest.Update_PIN = function() {};

/**
 * @param {{new_pin:string, old_pin:string}} data
 */
browserTest.Update_PIN.prototype.run = function(data) {
  var LOGIN_BACKOFF_WAIT = 2000;
  // Input validation
  browserTest.expect(typeof data.new_pin == 'string');
  browserTest.expect(typeof data.old_pin == 'string');
  browserTest.expect(data.new_pin != data.old_pin,
                     'The new PIN and the old PIN cannot be the same');

  this.changePIN_(data.new_pin).then(
    browserTest.connectMe2Me
  ).then(function(){
    return browserTest.enterPIN(data.old_pin, true /* expectError*/);
  }).then(
    // Sleep for two seconds to allow for the login backoff logic to reset.
    base.Promise.sleep.bind(null, LOGIN_BACKOFF_WAIT)
  ).then(
    browserTest.connectMe2Me
  ).then(function(){
    return browserTest.enterPIN(data.new_pin, false /* expectError*/)
  }).then(
    // Clean up the test by disconnecting and changing the PIN back
    browserTest.disconnect
  ).then(
    // The PIN must be restored regardless of success or failure.
    this.changePIN_.bind(this, data.old_pin),
    this.changePIN_.bind(this, data.old_pin)
  ).then(
    // On fulfilled.
    browserTest.pass,
    // On rejected.
    browserTest.fail
  );
};

/**
 * @param {string} newPin
 * @return {Promise}
 */
browserTest.Update_PIN.prototype.changePIN_ = function(newPin) {
  var AppMode = remoting.AppMode;
  var HOST_RESTART_WAIT = 10000;
  browserTest.clickOnControl('.change-daemon-pin');
  return browserTest.setupPIN(newPin);
};
