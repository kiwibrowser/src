// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * Mock implementation of ClientPlugin for testing.
 */

/** @suppress {duplicate} */
var remoting = remoting || {};

(function() {

'use strict';

/**
 * @constructor
 * @implements {remoting.ClientPlugin}
 */
remoting.MockClientPlugin = function() {
  /** @private {Element} */
  this.container_ = null;
  /** @private */
  this.element_ = /** @type {HTMLElement} */ (document.createElement('div'));
  this.element_.style.backgroundImage = 'linear-gradient(45deg, blue, red)';
  /** @private */
  this.hostDesktop_ =
      new remoting.ClientPlugin.HostDesktopImpl(this, base.doNothing);
  /** @private */
  this.extensions_ = new remoting.ProtocolExtensionManager(base.doNothing);

  /** @private {remoting.CredentialsProvider} */
  this.credentials_ = null;

  /** @private {remoting.ClientPlugin.ConnectionEventHandler} */
  this.connectionEventHandler_ = null;

  // Fake initialization result to return.
  this.mock$initializationResult = true;

  // Fake capabilities to return.
  this.mock$capabilities = [
      remoting.ClientSession.Capability.SEND_INITIAL_RESOLUTION,
      remoting.ClientSession.Capability.RATE_LIMIT_RESIZE_REQUESTS
  ];

  /** @private */
  this.onConnectDeferred_ = new base.Deferred();

  /** @private */
  this.onDisposedDeferred_ = new base.Deferred();

  /**
   * @private {?function(remoting.MockClientPlugin,
   *                     remoting.ClientSession.State)}
   */
  this.mock$onPluginStatusChanged_ = null;
};

remoting.MockClientPlugin.prototype.dispose = function() {
  this.container_.removeChild(this.element_);
  this.element_ = null;
  this.connectionStatusUpdateHandler_ = null;
  this.onDisposedDeferred_.resolve();
};

remoting.MockClientPlugin.prototype.extensions = function() {
  return this.extensions_;
};

remoting.MockClientPlugin.prototype.hostDesktop = function() {
  return this.hostDesktop_;
};

remoting.MockClientPlugin.prototype.element = function() {
  return this.element_;
};

remoting.MockClientPlugin.prototype.initialize = function() {
  if (this.mock$initializationResult) {
    return Promise.resolve();
  } else {
    return Promise.reject(
        new remoting.Error(remoting.Error.Tag.MISSING_PLUGIN));
  }
};


remoting.MockClientPlugin.prototype.connect =
    function(host, localJid, credentialsProvider) {
  this.credentials_ = credentialsProvider;
  this.onConnectDeferred_.resolve();
};

remoting.MockClientPlugin.prototype.injectKeyCombination = function(keys) {};

remoting.MockClientPlugin.prototype.injectKeyEvent =
    function(key, down) {};

remoting.MockClientPlugin.prototype.setRemapKeys = function(remappings) {};

remoting.MockClientPlugin.prototype.remapKey = function(from, to) {};

remoting.MockClientPlugin.prototype.releaseAllKeys = function() {};

remoting.MockClientPlugin.prototype.onIncomingIq = function(iq) {};

remoting.MockClientPlugin.prototype.isSupportedVersion = function() {
  return true;
};

remoting.MockClientPlugin.prototype.hasFeature = function(feature) {
  return false;
};

remoting.MockClientPlugin.prototype.hasCapability = function(capability) {
  return this.mock$capabilities.indexOf(capability) !== -1;
};

remoting.MockClientPlugin.prototype.sendClipboardItem =
    function(mimeType, item) {};

remoting.MockClientPlugin.prototype.enableTouchEvents =
    function(enable) {};

remoting.MockClientPlugin.prototype.requestPairing =
    function(clientName, onDone) {};

remoting.MockClientPlugin.prototype.allowMouseLock = function() {};

remoting.MockClientPlugin.prototype.pauseAudio = function(pause) {};

remoting.MockClientPlugin.prototype.pauseVideo = function(pause) {};

remoting.MockClientPlugin.prototype.getPerfStats = function() {
  var result = new remoting.ClientSession.PerfStats;
  result.videoBandwidth = 999;
  result.videoFrameRate = 60;
  result.captureLatency = 10;
  result.encodeLatency = 10;
  result.decodeLatency = 10;
  result.renderLatency = 10;
  result.roundtripLatency = 10;
  return result;
};

remoting.MockClientPlugin.prototype.setConnectionEventHandler =
    function(handler) {
  this.connectionEventHandler_ = handler;
};

remoting.MockClientPlugin.prototype.setMouseCursorHandler =
    function(handler) {};

remoting.MockClientPlugin.prototype.setClipboardHandler = function(handler) {};

remoting.MockClientPlugin.prototype.setDebugDirtyRegionHandler =
    function(handler) {};

/** @param {Element} container */
remoting.MockClientPlugin.prototype.mock$setContainer = function(container) {
  this.container_ = container;
  this.container_.appendChild(this.element_);
};

/** @return {Promise} */
remoting.MockClientPlugin.prototype.mock$onConnect = function() {
  this.onConnectDeferred_ = new base.Deferred();
  return this.onConnectDeferred_.promise();
};

/**
 * @return {Promise} Returns a promise that will resolve when the plugin is
 *     disposed.
 */
remoting.MockClientPlugin.prototype.mock$onDisposed = function() {
  this.onDisposedDeferred_ = new base.Deferred();
  return this.onDisposedDeferred_.promise();
};

/**
 * @param {remoting.ClientSession.State} status
 * @param {remoting.ClientSession.ConnectionError=} opt_error
 */
remoting.MockClientPlugin.prototype.mock$setConnectionStatus = function(
    status, opt_error) {
  console.assert(this.connectionEventHandler_ !== null,
                 '|connectionEventHandler_| is null.');
  var PluginError = remoting.ClientSession.ConnectionError;
  var error = opt_error ? opt_error : PluginError.NONE;
  this.connectionEventHandler_.onConnectionStatusUpdate(status, error);
  if (this.mock$onPluginStatusChanged_) {
    this.mock$onPluginStatusChanged_(this, status);
  }
};

/**
 * @param {remoting.ChromotingEvent.AuthMethod} authMethod
 * @return {Promise}
 */
remoting.MockClientPlugin.prototype.mock$authenticate = function(authMethod) {
  var AuthMethod = remoting.ChromotingEvent.AuthMethod;
  var deferred = new base.Deferred();

  var that = this;
  switch(authMethod) {
    case AuthMethod.PIN:
    case AuthMethod.ACCESS_CODE:
      this.credentials_.getPIN(true).then(function() {
        deferred.resolve();
      });
      break;
    case AuthMethod.THIRD_PARTY:
      this.credentials_.getThirdPartyToken(
          'fake_token_url', 'fake_host_publicKey', 'fake_scope'
      ).then(function() {
        deferred.resolve();
      });
      break;
    case AuthMethod.PINLESS:
      this.credentials_.getPairingInfo();
      deferred.resolve();
  }
  return deferred.promise();
};

/**
 * @param {?function(remoting.MockClientPlugin, remoting.ClientSession.State)}
 *    callback
 */
remoting.MockClientPlugin.prototype.mock$setPluginStatusChanged =
    function(callback) {
  this.mock$onPluginStatusChanged_ = callback;
};

/**
 * @param {remoting.ChromotingEvent.AuthMethod} authMethod
 */
remoting.MockClientPlugin.prototype.mock$useDefaultBehavior =
    function(authMethod) {
  var that = this;
  var State = remoting.ClientSession.State;
  this.mock$onConnect().then(function() {
    that.mock$setConnectionStatus(State.CONNECTING);
    return that.mock$authenticate(authMethod);
  }).then(function() {
    that.mock$setConnectionStatus(State.AUTHENTICATED);
  }).then(function() {
    that.mock$setConnectionStatus(State.CONNECTED);
  });
};

/**
 * @param {remoting.ClientSession.ConnectionError} error
 * @param {remoting.ChromotingEvent.AuthMethod=} opt_authMethod
 */
remoting.MockClientPlugin.prototype.mock$returnErrorOnConnect =
    function(error, opt_authMethod){
  var that = this;
  var State = remoting.ClientSession.State;
  this.mock$onConnect().then(function() {
    that.mock$setConnectionStatus(State.CONNECTING);
    var authMethod = opt_authMethod ? opt_authMethod :
                                      remoting.ChromotingEvent.AuthMethod.PIN;
    return that.mock$authenticate(authMethod);
  }).then(function() {
    that.mock$setConnectionStatus(State.FAILED, error);
  });
};

/**
 * @constructor
 * @implements {remoting.ClientPluginFactory}
 */
remoting.MockClientPluginFactory = function() {
  /** @private {?remoting.MockClientPlugin} */
  this.plugin_ = null;

  /**
   * @private {?function(remoting.MockClientPlugin)}
   */
  this.onPluginCreated_ = null;

  /**
   * @private {?function(remoting.MockClientPlugin,
   *                     remoting.ClientSession.State)}
   */
  this.onPluginStatusChanged_ = null;
};

remoting.MockClientPluginFactory.prototype.createPlugin =
    function(container, capabilities) {
  this.plugin_ = new remoting.MockClientPlugin();
  this.plugin_.mock$setContainer(container);
  this.plugin_.mock$capabilities = capabilities;

  // Notify the listeners on plugin creation.
  if (Boolean(this.onPluginCreated_)) {
    this.onPluginCreated_(this.plugin_);
  } else {
    this.plugin_.mock$useDefaultBehavior(
        remoting.ChromotingEvent.AuthMethod.PIN);
  }

  // Listens for plugin status changed.
  if (this.onPluginStatusChanged_) {
    this.plugin_.mock$setPluginStatusChanged(this.onPluginStatusChanged_);
  }
  return this.plugin_;
};

/**
 * Register a callback to configure the plugin before it returning to the
 * caller.
 *
 * @param {function(remoting.MockClientPlugin)} callback
 */
remoting.MockClientPluginFactory.prototype.mock$setPluginCreated =
    function(callback) {
  this.onPluginCreated_ = callback;
};

/**
 * @param {?function(remoting.MockClientPlugin, remoting.ClientSession.State)}
 *    callback
 */
remoting.MockClientPluginFactory.prototype.mock$setPluginStatusChanged =
    function(callback) {
  this.onPluginStatusChanged_ = callback;
};


/** @return {remoting.MockClientPlugin} */
remoting.MockClientPluginFactory.prototype.plugin = function() {
  return this.plugin_;
};

remoting.MockClientPluginFactory.prototype.preloadPlugin = function() {};

/**
 * A class that sets up all the dependencies required for mocking a connection.
 *
 * @constructor
 */
remoting.MockConnection = function() {
  /** @private */
  this.originalPluginFactory_ = remoting.ClientPlugin.factory;

  /** @private */
  this.pluginFactory_ = new remoting.MockClientPluginFactory();
  remoting.ClientPlugin.factory = this.pluginFactory_;

  /** @private */
  this.mockSignalStrategy_ = new remoting.MockSignalStrategy(
      'fake_jid', remoting.SignalStrategy.Type.XMPP);

  /** @private {sinon.TestStub} */
  this.createSignalStrategyStub_ =
      sinon.stub(remoting.SignalStrategy, 'create');
  this.createSignalStrategyStub_.returns(this.mockSignalStrategy_);

  /** @private */
  this.originalIdentity_ = remoting.identity;
  remoting.identity = new remoting.Identity();
  var identityStub = sinon.stub(remoting.identity, 'getUserInfo');
  identityStub.returns(Promise.resolve({email: 'email', userName: 'userName'}));

  /** @private */
  this.originalSettings_ = remoting.settings;
  remoting.settings = new remoting.Settings();
};

/** @return {remoting.MockClientPluginFactory} */
remoting.MockConnection.prototype.pluginFactory = function() {
  return this.pluginFactory_;
};

/** @return {remoting.MockClientPlugin} */
remoting.MockConnection.prototype.plugin = function() {
  return this.pluginFactory_.plugin();
};

/** @return {remoting.MockSignalStrategy} */
remoting.MockConnection.prototype.signalStrategy = function() {
  return this.mockSignalStrategy_;
};

remoting.MockConnection.prototype.restore = function() {
  remoting.settings = this.originalSettings_;
  remoting.identity = this.originalIdentity_;
  remoting.ClientPlugin.factory = this.originalPluginFactory_;
  this.createSignalStrategyStub_.restore();
};

})();
