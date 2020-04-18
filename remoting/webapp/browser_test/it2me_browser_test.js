// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * Browser test for the scenario below:
 * 1. Generates an access code.
 * 2. Launches another chromoting app instance.
 * 3. Connects with the generated access code.
 * 4. Verifies that the session is connected.
 */

'use strict';

/** @constructor */
browserTest.ConnectIt2Me = function() {};

/**
 * @param {{pin: string, accessCode: string}} data
 */
browserTest.ConnectIt2Me.prototype.run = function(data) {
  browserTest.expect(typeof data.accessCode == 'string',
                     'The access code should be an non-empty string');
  browserTest.clickOnControl('get-started-it2me');
  var that = this;
  browserTest.ConnectIt2Me.clickOnAccessButton().then(function() {
    return that.enterAccessCode_(data.accessCode);
  }).then(function() {
    return browserTest.disconnect();
  }).then(function() {
    browserTest.pass();
  }, function(/** * */reason) {
    browserTest.fail(/** @type {Error} */(reason));
  });
};

/** @return {Promise} */
browserTest.ConnectIt2Me.clickOnAccessButton = function() {
  browserTest.clickOnControl('access-mode-button');
  return browserTest.onUIMode(remoting.AppMode.CLIENT_UNCONNECTED);
};

/**
 * @param {string} code
 * @return {Promise}
 * @private
 */
browserTest.ConnectIt2Me.prototype.enterAccessCode_ = function(code) {
  document.getElementById('access-code-entry').value = code;
  browserTest.clickOnControl('connect-button');
  return browserTest.expectConnected();
};

/** @constructor */
browserTest.InvalidAccessCode = function() {};

/**
 * @param {{pin: string, accessCode: string}} data
 */
browserTest.InvalidAccessCode.prototype.run = function(data) {
  browserTest.expect(typeof data.accessCode == 'string',
                     'The access code should be an non-empty string');
  browserTest.ConnectIt2Me.clickOnAccessButton().then(function() {
    document.getElementById('access-code-entry').value = data.accessCode;
    browserTest.clickOnControl('connect-button');
    var ErrorTag = remoting.Error.Tag;
    return browserTest.expectConnectionError(
        remoting.DesktopRemoting.Mode.IT2ME,
        [ErrorTag.INVALID_ACCESS_CODE, ErrorTag.HOST_IS_OFFLINE]);
  }).then(function() {
    browserTest.pass();
  }, function(/** * */reason) {
    browserTest.fail(/** @type {Error} */(reason));
  });
};

/** @constructor */
browserTest.GetAccessCode = function() {};

browserTest.GetAccessCode.prototype.run = function() {
  browserTest.clickOnControl('get-started-it2me');

  // Wait for the email address of the local user to become available.  The
  // email address is required in an It2Me connection for domain policy
  // enforcement. TODO:(kelvinp) Fix this awkward behavior in the production
  // code so that this  hack is no longer required.
  remoting.identity.getUserInfo().then(function(info) {
    browserTest.clickOnControl('share-button');
  }).then(function(){
    return browserTest.onUIMode(remoting.AppMode.HOST_WAITING_FOR_CONNECTION);
  }).then(function() {
    var accessCode = document.getElementById('access-code-display').innerText;
    var numericAccessCode = parseFloat(accessCode);
    browserTest.expect(accessCode.length === 12,
                       "The access code should be 12 digits long.");
    browserTest.expect(
        Number.isInteger(numericAccessCode) && numericAccessCode > 0,
        "The access code should be a positive integer.");
    browserTest.pass();
  }).catch(function(/** Error */ reason) {
    browserTest.fail(reason);
  });
};

/** @constructor */
browserTest.CancelShare = function() {};

browserTest.CancelShare.prototype.run = function() {
  browserTest.clickOnControl('cancel-share-button');
  browserTest.onUIMode(remoting.AppMode.HOST_SHARE_FINISHED).then(function() {
    browserTest.pass();
  }).catch(function(/** Error */ reason) {
    browserTest.fail(reason);
  });
};
