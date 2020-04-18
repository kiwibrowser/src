// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

'use strict';

/** @constructor */
browserTest.ConnectToLocalHost = function() {};

/**
 * @param {{pin:string}} data
 */
browserTest.ConnectToLocalHost.prototype.run = function(data) {
  browserTest.expect(typeof data.pin == 'string', 'The PIN must be a string');
  browserTest.connectMe2Me().then(function() {
    return browserTest.enterPIN(data.pin);
  }).then(function() {
    return browserTest.disconnect();
  }).then(function() {
    browserTest.pass();
  }).catch(function(/** Error */ reason) {
    browserTest.fail(reason);
  });
};


/** @constructor */
browserTest.AliveOnLostFocus = function() {};

/**
 * @param {{pin:string}} data
 */
browserTest.AliveOnLostFocus.prototype.run = function(data) {
  browserTest.expect(typeof data.pin == 'string', 'The PIN must be a string');
  browserTest.connectMe2Me().then(function() {
    return browserTest.enterPIN(data.pin);
  }).then(function() {
    chrome.app.window.current().minimize();
    // Wait for a few seconds for app to process any notifications it
    // would have got from minimizing.
    return base.Promise.sleep(4000);
  }).then(function() {
    chrome.app.window.current().restore();
  }).then(function() {
    // Validate that the session is still active.
    if (remoting.currentMode !== remoting.AppMode.IN_SESSION) {
      throw new Error(
          'Unexpected session disconnect when the app is minimized.');
    }
  }).then(function() {
    return browserTest.disconnect();
  }).then(function() {
    browserTest.pass();
  }).catch(function(/** Error */ reason) {
    browserTest.fail(reason);
  });
};

})();
