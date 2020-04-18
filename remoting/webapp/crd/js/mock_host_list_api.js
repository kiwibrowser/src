// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * Mock implementation of remoting.HostList
 */

'use strict';

/** @suppress {duplicate} */
var remoting = remoting || {};

/**
 * @constructor
 * @implements {remoting.HostListApi}
 */
remoting.MockHostListApi = function() {
  /**
   * The auth code value for the |register| method to return, or null
   * if it should fail.
   * @type {?string}
   */
  this.authCodeFromRegister = null;

  /**
   * The email value for the |register| method to return, or null if
   * it should fail.
   * @type {?string}
   */
  this.emailFromRegister = null;

  /**
   * The host ID to return from register(), or null if it should fail.
   * @type {?string}
   */
  this.hostIdFromRegister = null;

  /** @type {!Array<!remoting.Host>} */
  this.hosts = [];
};

/**
 * Creates and adds a new mock host.
 *
 * @param {string} hostId The ID of the new host to add.
 * @return {!remoting.Host} the new mock host
 */
remoting.MockHostListApi.prototype.addMockHost = function(hostId) {
  var newHost = new remoting.Host(hostId);
  this.hosts.push(newHost);
  return newHost;
};

/** @override */
remoting.MockHostListApi.prototype.register = function(
    hostName, publicKey, hostClientId) {
  if (this.authCodeFromRegister === null || this.emailFromRegister === null) {
    return Promise.reject(
        new remoting.Error(
            remoting.Error.Tag.REGISTRATION_FAILED,
            'MockHostListApi.register'));
  } else {
    return Promise.resolve({
      authCode: this.authCodeFromRegister,
      email: this.emailFromRegister,
      hostId: this.hostIdFromRegister
    });
  }
};

/** @override */
remoting.MockHostListApi.prototype.get = function() {
  return Promise.resolve(this.hosts);
};

/**
 * @override
 * @param {string} hostId
 * @param {string} hostName
 * @param {string} hostPublicKey
 */
remoting.MockHostListApi.prototype.put =
    function(hostId, hostName, hostPublicKey) {
  /** @type {remoting.MockHostListApi} */
  var that = this;
  return new Promise(function(resolve, reject) {
    for (var i = 0; i < that.hosts.length; ++i) {
      /** type {remoting.Host} */
      var host = that.hosts[i];
      if (host.hostId == hostId) {
        host.hostName = hostName;
        host.hostPublicKey = hostPublicKey;
        resolve(undefined);
        return;
      }
    }
    console.error('PUT request for unknown host: ' + hostId +
                  ' (' + hostName + ')');
    reject(remoting.Error.unexpected());
  });
};

/**
 * @override
 * @param {string} hostId
 */
remoting.MockHostListApi.prototype.remove = function(hostId) {
  /** @type {remoting.MockHostListApi} */
  var that = this;
  return new Promise(function(resolve, reject) {
    for (var i = 0; i < that.hosts.length; ++i) {
      var host = that.hosts[i];
      if (host.hostId == hostId) {
        that.hosts.splice(i, 1);
        resolve(undefined);
        return;
      }
    }
    console.error('DELETE request for unknown host: ' + hostId);
    reject(remoting.Error.unexpected());
  });
};

/** @override */
remoting.MockHostListApi.prototype.getSupportHost = function(supportId) {
  return Promise.resolve(this.hosts[0]);
};

/**
 * @param {boolean} active
 */
remoting.MockHostListApi.setActive = function(active) {
  remoting.HostListApi.setInstance(
      active ? new remoting.MockHostListApi() : null);
};
