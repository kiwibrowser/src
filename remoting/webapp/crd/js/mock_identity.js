// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * Mock implementation of chrome.identity.
 */

'use strict';

/** @suppress {duplicate} */
var remoting = remoting || {};

/**
 * @constructor
 */
remoting.MockIdentity = function() {
  /**
   * @type {remoting.MockIdentity.AccessToken}
   * @private
   */
  this.accessToken_ = remoting.MockIdentity.AccessToken.NONE;
};

/**
 * @param {Object} details
 * @param {function(string)} callback
 */
remoting.MockIdentity.prototype.getAuthToken = function(details, callback) {
  window.setTimeout(callback.bind(null, this.accessToken_), 0);
};

/**
 * @param {Object} details
 * @param {function()} callback
 */
remoting.MockIdentity.prototype.removeCachedAuthToken =
    function(details, callback) {
  window.setTimeout(callback, 0);
};

/**
 * @param {Object} details
 * @param {function()} callback
 */
remoting.MockIdentity.prototype.launchWebAuthFlow =
    function(details, callback) {
  // TODO(jamiewalch): Work out how to test third-party auth.
  console.error('No mock implementation for launchWebAuthFlow.');
};


/** @enum {string} */
remoting.MockIdentity.AccessToken = {
  VALID: 'valid-token',
  INVALID: 'invalid-token',
  NONE: ''
};

/**
 * @param {remoting.MockIdentity.AccessToken} accessToken
 */
remoting.MockIdentity.prototype.setAccessToken = function(accessToken) {
  this.accessToken_ = accessToken;
};

/**
 * @param {string} token
 * @param {Function} onDone
 * @param {function(!remoting.Error)} onError
 * @param {Array<*>} values
 */
remoting.MockIdentity.validateTokenAndCall =
    function(token, onDone, onError, values) {
  if (token == remoting.MockIdentity.AccessToken.VALID) {
    window.setTimeout(
        onDone.apply.bind(onDone, null, values),
        0);
  } else {
    window.setTimeout(
        onError.bind(null, new remoting.Error(
            remoting.Error.Tag.AUTHENTICATION_FAILED)),
        0);
  }
};

/**
 * @param {Function} onDone
 * @param {function(!remoting.Error)} onError
 * @param {Array<*>} values
 */
remoting.MockIdentity.prototype.validateTokenAndCall =
    function(onDone, onError, values) {
  remoting.MockIdentity.validateTokenAndCall(
      this.accessToken_, onDone, onError, values);
};

/**
 * @param {boolean} active
 */
remoting.MockIdentity.setActive = function(active) {
  chrome['identity'] =
      active ? remoting.mockIdentity : remoting.savedIdentityApi;
};

/** @type {Object} */
remoting.savedIdentityApi = chrome.identity;

/** @type {remoting.MockIdentity} */
remoting.mockIdentity = new remoting.MockIdentity();
