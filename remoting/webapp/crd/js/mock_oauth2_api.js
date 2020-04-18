// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * Mock implementation of remoting.OAuth2Api
 */

'use strict';

/** @suppress {duplicate} */
var remoting = remoting || {};

/**
 * @constructor
 * @implements {remoting.OAuth2Api}
 */
remoting.MockOAuth2Api = function() {
  /**
   * @type {string}
   * @private
   */
  this.email_ = 'fake-email@test-user.com';

  /**
   * @type {string}
   * @private
   */
  this.fullName_ = 'Fake User, Esq.';
};

/**
 * @param {function(string, number): void} onDone
 * @param {function(!remoting.Error):void} onError
 * @param {string} clientId
 * @param {string} clientSecret
 * @param {string} refreshToken
 * @return {void} Nothing.
 */
remoting.MockOAuth2Api.prototype.refreshAccessToken = function(
    onDone, onError, clientId, clientSecret, refreshToken) {
  window.setTimeout(
      onDone.bind(null, remoting.MockIdentity.AccessToken.VALID, 60 * 60),
      0);
};

/**
 * @param {function(string, string, number): void} onDone
 * @param {function(!remoting.Error):void} onError
 * @param {string} clientId
 * @param {string} clientSecret
 * @param {string} code
 * @param {string} redirectUri
 * @return {void} Nothing.
 */
remoting.MockOAuth2Api.prototype.exchangeCodeForTokens = function(
    onDone, onError, clientId, clientSecret, code, redirectUri) {
  window.setTimeout(
      onDone.bind(null, '', remoting.MockIdentity.AccessToken.VALID, 60 * 60),
      0);
};

/**
 * @param {function(string,string)} onDone
 * @param {function(!remoting.Error)} onError
 * @param {string} token
 */
remoting.MockOAuth2Api.prototype.getEmail = function(onDone, onError, token) {
  remoting.MockIdentity.validateTokenAndCall(
      token, onDone, onError, [this.email_]);
};

/**
 * @param {function(string,string)} onDone
 * @param {function(!remoting.Error)} onError
 * @param {string} token
 */
remoting.MockOAuth2Api.prototype.getUserInfo =
    function(onDone, onError, token) {
  remoting.MockIdentity.validateTokenAndCall(
      token, onDone, onError, [this.email_, this.fullName_]);
};


/**
 * @param {boolean} active
 */
remoting.MockOAuth2Api.setActive = function(active) {
  remoting.oauth2Api = active ? new remoting.MockOAuth2Api()
                              : new remoting.OAuth2ApiImpl();
};
