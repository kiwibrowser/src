// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview A mock version of HostDaemonFacade.  Internally all
 * delays are implemented with promises, so SpyPromise can be used to
 * wait out delays.
 *
 * By default, every method fails.  Methods can be individually set to
 * pass specific values to their onDone arguments by setting member
 * variables of the mock object.
 *
 * When methods fail, they set the detail field of the remoting.Error
 * object to the name of the method that failed.
 */

/** @suppress {duplicate} */
var remoting = remoting || {};

(function() {

'use strict';

/**
 * By default, all methods fail.
 * @constructor
 */
remoting.MockHostDaemonFacade = function() {
  /** @type {Array<remoting.HostController.Feature>} */
  this.features = [];

  /** @type {?string} */
  this.hostName = null;

  /**
   * A function to generate a fake PIN hash given a host ID and a PIN.
   * @type {?function(string,string):string}
   */
  this.pinHashFunc = null;

  /** @type {?string} */
  this.privateKey = null;

  /** @type {?string} */
  this.publicKey = null;

  /** @type {Object} */
  this.daemonConfig = null;

  /** @type {?string} */
  this.daemonVersion = null;

  /** @type {?boolean} */
  this.consentSupported = null;

  /** @type {?boolean} */
  this.consentAllowed = null;

  /** @type {?boolean} */
  this.consentSetByPolicy = null;

  /** @type {?remoting.HostController.AsyncResult} */
  this.startDaemonResult = null;

  /** @type {?remoting.HostController.AsyncResult} */
  this.stopDaemonResult = null;

  /** @type {?remoting.HostController.State} */
  this.daemonState = null;

  /** @type {?remoting.HostController.AsyncResult} */
  this.updateDaemonConfigResult = null;

  /** @type {Array<remoting.PairedClient>} */
  this.pairedClients = null;

  /** @type {?string} */
  this.hostClientId = null;

  /** @type {?string} */
  this.userEmail = null;

  /** @type {?string} */
  this.refreshToken = null;
};

/**
 * @param {remoting.HostController.Feature} feature
 * @return {!Promise<boolean>}
 */
remoting.MockHostDaemonFacade.prototype.hasFeature = function(feature) {
  var that = this;
  return Promise.resolve().then(function() {
    return that.features.indexOf(feature) >= 0;
  });
};

/**
 * @return {!Promise<string>}
 */
remoting.MockHostDaemonFacade.prototype.getHostName = function() {
  var that = this;
  return Promise.resolve().then(function() {
    if (that.hostName === null) {
      throw remoting.Error.unexpected('getHostName');
    } else {
      return that.hostName;
    }
  });
};

/**
 * @param {string} hostId
 * @param {string} pin
 * @return {!Promise<string>}
 */
remoting.MockHostDaemonFacade.prototype.getPinHash = function(hostId, pin) {
  var that = this;
  return Promise.resolve().then(function() {
    if (that.pinHashFunc === null) {
      throw remoting.Error.unexpected('getPinHash');
    } else {
      return that.pinHashFunc(hostId, pin);
    }
  });
};

/**
 * @return {!Promise<{privateKey:string, publicKey:string}>}
 */
remoting.MockHostDaemonFacade.prototype.generateKeyPair = function() {
  var that = this;
  return Promise.resolve().then(function() {
    if (that.privateKey === null || that.publicKey === null) {
      throw remoting.Error.unexpected('generateKeyPair');
    } else {
      return {
        privateKey: that.privateKey,
        publicKey: that.publicKey
      };
    }
  });
};

/**
 * @param {Object} config The new config parameters.
 * @return {!Promise<remoting.HostController.AsyncResult>}
 */
remoting.MockHostDaemonFacade.prototype.updateDaemonConfig = function(config) {
  var that = this;
  return Promise.resolve().then(function() {
    if (that.daemonConfig === null ||
        that.updateDaemonConfigResult === null ||
        'host_id' in config ||
        'xmpp_login' in config) {
      throw remoting.Error.unexpected('updateDaemonConfig');
    } else if (that.updateDaemonConfigResult !=
               remoting.HostController.AsyncResult.OK) {
      return that.updateDaemonConfigResult;
    } else {
      base.mix(that.daemonConfig, config);
      return remoting.HostController.AsyncResult.OK;
    }
  });
};

/**
 * @return {!Promise<Object>}
 */
remoting.MockHostDaemonFacade.prototype.getDaemonConfig = function() {
  var that = this;
  return Promise.resolve().then(function() {
    if (that.daemonConfig === null) {
      throw remoting.Error.unexpected('getDaemonConfig');
    } else {
      return that.daemonConfig;
    }
  });
};

/**
 * @return {!Promise<string>}
 */
remoting.MockHostDaemonFacade.prototype.getDaemonVersion = function() {
  var that = this;
  return Promise.resolve().then(function() {
    if (that.daemonVersion === null) {
      throw remoting.Error.unexpected('getDaemonVersion');
    } else {
      return that.daemonVersion;
    }
  });
};

/**
 * @return {!Promise<remoting.UsageStatsConsent>}
 */
remoting.MockHostDaemonFacade.prototype.getUsageStatsConsent = function() {
  var that = this;
  return Promise.resolve().then(function() {
    if (that.consentSupported === null ||
        that.consentAllowed === null ||
        that.consentSetByPolicy === null) {
      throw remoting.Error.unexpected('getUsageStatsConsent');
    } else {
      return {
        supported: that.consentSupported,
        allowed: that.consentAllowed,
        setByPolicy: that.consentSetByPolicy
      };
    }
  });
};

/**
 * @param {Object} config
 * @param {boolean} consent Consent to report crash dumps.
 * @return {!Promise<remoting.HostController.AsyncResult>}
 */
remoting.MockHostDaemonFacade.prototype.startDaemon =
    function(config, consent) {
  var that = this;
  return Promise.resolve().then(function() {
    if (that.startDaemonResult === null) {
      throw remoting.Error.unexpected('startDaemon');
    } else {
      return that.startDaemonResult;
    }
  });
};

/**
 * @return {!Promise<remoting.HostController.AsyncResult>}
 */
remoting.MockHostDaemonFacade.prototype.stopDaemon =
    function() {
  var that = this;
  return Promise.resolve().then(function() {
    if (that.stopDaemonResult === null) {
      throw remoting.Error.unexpected('stopDaemon');
    } else {
      return that.stopDaemonResult;
    }
  });
};

/**
 * @return {!Promise<remoting.HostController.State>}
 */
remoting.MockHostDaemonFacade.prototype.getDaemonState =
    function() {
  var that = this;
  return Promise.resolve().then(function() {
    if (that.daemonState === null) {
      throw remoting.Error.unexpected('getDaemonState');
    } else {
      return that.daemonState;
    }
  });
};

/**
 * @return {!Promise<Array<remoting.PairedClient>>}
 */
remoting.MockHostDaemonFacade.prototype.getPairedClients =
    function() {
  var that = this;
  return Promise.resolve().then(function() {
    if (that.pairedClients === null) {
      throw remoting.Error.unexpected('getPairedClients');
    } else {
      return that.pairedClients;
    }
  });
};

/**
 * @return {!Promise<boolean>}
 */
remoting.MockHostDaemonFacade.prototype.clearPairedClients = function() {
  var that = this;
  return Promise.resolve().then(function() {
    if (that.pairedClients === null) {
      throw remoting.Error.unexpected('clearPairedClients');
    } else {
      that.pairedClients = [];
      return true;  // TODO(jrw): Not always correct.
    }
  });
};

/**
 * @param {string} client
 * @return {!Promise<boolean>}
 */
remoting.MockHostDaemonFacade.prototype.deletePairedClient = function(client) {
  var that = this;
  return Promise.resolve().then(function() {
    if (that.pairedClients === null) {
      throw remoting.Error.unexpected('deletePairedClient');
    } else {
      that.pairedClients = that.pairedClients.filter(function(c) {
        return c['clientId'] != client;
      });
      return true;  // TODO(jrw):  Not always correct.
    }
  });
};

/**
 * @return {!Promise<string>}
 */
remoting.MockHostDaemonFacade.prototype.getHostClientId = function() {
  var that = this;
  return Promise.resolve().then(function() {
    if (that.hostClientId === null) {
      throw remoting.Error.unexpected('getHostClientId');
    } else {
      return that.hostClientId;
    }
  });
};

/**
 * @param {string} authorizationCode
 * @return {!Promise<{userEmail:string, refreshToken:string}>}
 */
remoting.MockHostDaemonFacade.prototype.getCredentialsFromAuthCode =
    function(authorizationCode) {
  var that = this;
  return Promise.resolve().then(function() {
    if (that.userEmail === null || that.refreshToken === null) {
      throw remoting.Error.unexpected('getCredentialsFromAuthCode');
    } else {
      return {
        userEmail: that.userEmail,
        refreshToken: that.refreshToken
      };
    }
  });
};

})();
