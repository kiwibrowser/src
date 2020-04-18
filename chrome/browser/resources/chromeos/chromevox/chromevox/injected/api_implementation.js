// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Implentation of ChromeVox's public API.
 *
 */

goog.provide('cvox.ApiImplementation');

goog.require('cvox.ChromeVox');
goog.require('cvox.ExtensionBridge');
goog.require('cvox.ScriptInstaller');

/**
 * @constructor
 */
cvox.ApiImplementation = function() {};

/**
 * The message between content script and the page that indicates the
 * connection to the background page has been lost.
 * @type {string}
 * @const
 */
cvox.ApiImplementation.DISCONNECT_MSG = 'cvox.Disconnect';

/**
 * Inject the API into the page and set up communication with it.
 * @param {function()=} opt_onload A function called when the script is loaded.
 */
cvox.ApiImplementation.init = function(opt_onload) {
  window.addEventListener('message', cvox.ApiImplementation.portSetup, true);
  var scripts = [window.chrome.extension.getURL('chromevox/injected/api.js')];

  var didInstall =
      cvox.ScriptInstaller.installScript(scripts, 'cvoxapi', opt_onload);
  if (!didInstall) {
    console.error('Unable to install api scripts');
  }

  cvox.ExtensionBridge.addDisconnectListener(function() {
    cvox.ApiImplementation.port.postMessage(
        cvox.ApiImplementation.DISCONNECT_MSG);
    cvox.ScriptInstaller.uninstallScript('cvoxapi');
  });
};

/**
 * This method is called when the content script receives a message from
 * the page.
 * @param {Event} event The DOM event with the message data.
 * @return {boolean} True if default event processing should continue.
 */
cvox.ApiImplementation.portSetup = function(event) {
  if (event.data == 'cvox.PortSetup') {
    cvox.ApiImplementation.port = event.ports[0];
    cvox.ApiImplementation.port.onmessage = function(event) {
      cvox.ApiImplementation.dispatchApiMessage(JSON.parse(event.data));
    };

    // Stop propagation since it was our message.
    event.stopPropagation();
    return false;
  }
  return true;
};

/**
 * Call the appropriate API function given a message from the page.
 * @param {*} message The message.
 */
cvox.ApiImplementation.dispatchApiMessage = function(message) {
  var method;
  switch (message['cmd']) {
    case 'speak':
      method = cvox.ApiImplementation.speak;
      break;
      break;
  }
  if (!method) {
    throw 'Unknown API call: ' + message['cmd'];
  }

  method.apply(cvox.ApiImplementation, message['args']);
};

/**
 * Sets endCallback in properties to call callbackId's function.
 * @param {Object} properties Speech properties to use for this utterance.
 * @param {number} callbackId The callback Id.
 * @private
 */
function setupEndCallback_(properties, callbackId) {
  var endCallback = function() {
    cvox.ApiImplementation.port.postMessage(JSON.stringify({'id': callbackId}));
  };
  if (properties) {
    properties['endCallback'] = endCallback;
  }
}

/**
 * Speaks the given string using the specified queueMode and properties.
 *
 * @param {number} callbackId The callback Id.
 * @param {string} textString The string of text to be spoken.
 * @param {number=} queueMode Valid modes are 0 for flush; 1 for queue.
 * @param {Object=} properties Speech properties to use for this utterance.
 */
cvox.ApiImplementation.speak = function(
    callbackId, textString, queueMode, properties) {
  if (!properties) {
    properties = {};
  }
  setupEndCallback_(properties, callbackId);
  var message = {
    'target': 'TTS',
    'action': 'speak',
    'text': textString,
    'queueMode': queueMode,
    'properties': properties
  };

  cvox.ExtensionBridge.send(message);
};
