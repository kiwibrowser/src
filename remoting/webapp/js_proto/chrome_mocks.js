// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains various mock objects for the chrome platform to make
// unit testing easier.

var chromeMocks = {};

(function(){

'use strict'

/**
 * @constructor
 * @extends {ChromeEvent}
 */
chromeMocks.Event = function() {
  this.listeners_ = [];
};

/** @param {Function} callback */
chromeMocks.Event.prototype.addListener = function(callback) {
  this.listeners_.push(callback);
};

/** @param {Function} callback */
chromeMocks.Event.prototype.removeListener = function(callback) {
  for (var i = 0; i < this.listeners_.length; i++) {
    if (this.listeners_[i] === callback) {
      this.listeners_.splice(i, 1);
      break;
    }
  }
};

/**
 * @param {...*} var_args
 * @return {void}
 * @suppress {reportUnknownTypes}
 */
chromeMocks.Event.prototype.mock$fire = function(var_args) {
  var params = Array.prototype.slice.call(arguments);
  this.listeners_.forEach(
      /** @param {Function} listener */
      function(listener){
        listener.apply(null, params);
      });
};

/** @type {Object} */
chromeMocks.runtime = {};

/** @constructor */
chromeMocks.runtime.Port = function() {
  /** @const */
  this.onMessage = new chromeMocks.Event();

  /** @const */
  this.onDisconnect = new chromeMocks.Event();

  /** @type {string} */
  this.name = '';

  /** @type {MessageSender} */
  this.sender = null;
};

chromeMocks.runtime.Port.prototype.disconnect = function() {};

/**
 * @param {Object} message
 */
chromeMocks.runtime.Port.prototype.postMessage = function(message) {};

/** @type {chromeMocks.Event} */
chromeMocks.runtime.onMessage = new chromeMocks.Event();

/** @type {chromeMocks.Event} */
chromeMocks.runtime.onMessageExternal = new chromeMocks.Event();

/** @type {chromeMocks.Event} */
chromeMocks.runtime.onSuspend = new chromeMocks.Event();

/**
 * @param {string?} extensionId
 * @param {*} message
 * @param {function(*)=} responseCallback
 */
chromeMocks.runtime.sendMessage = function(extensionId, message,
                                           responseCallback) {
  console.assert(
      extensionId === null,
      'The mock only supports sending messages to the same extension.');
  extensionId = chrome.runtime.id;
  Promise.resolve().then(function() {
    var message_copy = base.deepCopy(message);
    chromeMocks.runtime.onMessage.mock$fire(
        message_copy, {id: extensionId}, responseCallback);
  });
};

/**
 * Always returns the same mock port for given application name.
 * @param {string} application
 * @return {chromeMocks.runtime.Port}
 */
chromeMocks.runtime.connectNative = function(application) {
  var port = nativePorts[application];
  if (port === undefined) {
    port = new chromeMocks.runtime.Port();
    port.name = application;
    nativePorts[application] = port;
  }
  return port;
};

/** @type {Object<!chromeMocks.runtime.Port>} */
var nativePorts = null;

/** @type {string} */
chromeMocks.runtime.id = 'extensionId';

/** @type {Object} */
chromeMocks.runtime.lastError = {
  /** @type {string|undefined} */
  message: undefined
};

chromeMocks.runtime.getManifest = function() {
  return {
    version: 10,
    app: {
      background: true
    }
  };
};

// Sample implementation of chrome.StorageArea according to
// https://developer.chrome.com/apps/storage#type-StorageArea
/**
 * @constructor
 * @extends {StorageArea}
 */
chromeMocks.StorageArea = function() {
  /** @type {!Object} */
  this.storage_ = {};
};

/**
 * @param {Object|string} keys
 * @return {Array<string>}
 */
function getKeys(keys) {
  if (typeof keys === 'string') {
    return [keys];
  } else if (typeof keys === 'object') {
    var objectKeys = /** @type {!Object} */ (keys);
    return Object.keys(objectKeys);
  }
  return [];
}

chromeMocks.StorageArea.prototype.get = function(keys, onDone) {
  if (!keys) {
    // No keys are specified, returns the entire storage.
    var storageCopy = base.deepCopy(this.storage_);
    onDone(/** @type {!Object} */ (storageCopy));
    return;
  }

  var result = (typeof keys === 'object') ? keys : {};
  getKeys(keys).forEach(
      /** @param {string} key */
      function(key) {
        if (key in this.storage_) {
          result[key] = base.deepCopy(this.storage_[key]);
        }
      }, this);
  onDone(result);
};

chromeMocks.StorageArea.prototype.set = function(value, opt_onDone) {
  for (var key in value) {
    this.storage_[key] = base.deepCopy(value[key]);
  }
  if (opt_onDone) {
    opt_onDone();
  }
};

chromeMocks.StorageArea.prototype.remove = function(keys, opt_onDone) {
  getKeys(keys).forEach(
      /** @param {string} key */
      function(key) {
        delete this.storage_[key];
      }, this);
  if (opt_onDone) {
    opt_onDone();
  }
};

/** @return {!Object} */
chromeMocks.StorageArea.prototype.mock$getStorage = function() {
  return this.storage_;
};

chromeMocks.StorageArea.prototype.clear = function() {
  this.storage_ = {};
};

/** @type {Object} */
chromeMocks.storage = {};

/** @type {chromeMocks.StorageArea} */
chromeMocks.storage.local = new chromeMocks.StorageArea();


/** @constructor */
chromeMocks.Identity = function() {
  /** @private {string|undefined} */
  this.token_ = undefined;
};

/**
 * @param {Object} options
 * @param {function(string=):void} callback
 */
chromeMocks.Identity.prototype.getAuthToken = function(options, callback) {
  // Append the 'scopes' array, if present, to the dummy token.
  var token = this.token_;
  if (token !== undefined && options['scopes'] !== undefined) {
    token += JSON.stringify(options['scopes']);
  }
  // Don't use setTimeout because sinon mocks it.
  Promise.resolve().then(callback.bind(null, token));
};

/** @param {string} token */
chromeMocks.Identity.prototype.mock$setToken = function(token) {
  this.token_ = token;
};

chromeMocks.Identity.prototype.mock$clearToken = function() {
  this.token_ = undefined;
};

/** @type {chromeMocks.Identity} */
chromeMocks.identity;

/** @constructor */
chromeMocks.MetricsPrivate = function() {};

chromeMocks.MetricsPrivate.prototype.MetricTypeType = {
  HISTOGRAM_LOG: 'histogram-log',
  HISTOGRAM_LINEAR: 'histogram-linear',
};

chromeMocks.MetricsPrivate.prototype.recordValue = function() {};

/** @type {chromeMocks.MetricsPrivate} */
chromeMocks.metricsPrivate;

/** @constructor */
chromeMocks.I18n = function() {};

/**
 * @param {string} messageName
 * @param {(string|Array<string>)=} opt_args
 * @return {string}
 */
chromeMocks.I18n.prototype.getMessage = function(messageName, opt_args) {};

/**
 * @return {string}
 */
chromeMocks.I18n.prototype.getUILanguage = function() {};

/** @constructor */
chromeMocks.WindowManager = function() {
  this.current_ = new chromeMocks.AppWindow();
};

chromeMocks.WindowManager.prototype.current = function() {
  return this.current_;
};

/** @constructor */
chromeMocks.AppWindow = function() {};

var originals_ = null;

/**
 * Activates a list of Chrome components to mock
 */
chromeMocks.activate = function() {
  if (originals_) {
    throw new Error('chromeMocks.activate() can only be called once.');
  }
  originals_ = {};
  nativePorts = {};

  chromeMocks.i18n = new chromeMocks.I18n();
  chromeMocks.identity = new chromeMocks.Identity();
  chromeMocks.metricsPrivate = new chromeMocks.MetricsPrivate();

  ['identity', 'i18n', 'runtime', 'storage', 'metricsPrivate'].forEach(
    function(/** string */ component) {
      if (!chromeMocks[component]) {
        throw new Error('No mocks defined for chrome.' + component);
      }
      originals_[component] = chrome[component];
      chrome[component] = chromeMocks[component];
    });

  chrome.app['window'] = new chromeMocks.WindowManager();
};

chromeMocks.restore = function() {
  if (!originals_) {
    throw new Error('You must call activate() before restore().');
  }
  for (var components in originals_) {
    chrome[components] = originals_[components];
  }
  originals_ = null;
  nativePorts = null;
};

})();
