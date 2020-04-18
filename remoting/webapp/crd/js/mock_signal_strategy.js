// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/** @suppress {duplicate} */
var remoting = remoting || {};


/**
 * @param {string=} jid
 * @param {remoting.SignalStrategy.Type=} type
 *
 * @implements {remoting.SignalStrategy}
 * @constructor
 */
remoting.MockSignalStrategy = function(jid, type) {
  this.jid_ = (jid != undefined) ? jid : "jid@example.com";
  this.type_ = (type != undefined) ? type : remoting.SignalStrategy.Type.XMPP;
  this.onStateChangedCallback_ = null;

  /** @type {remoting.SignalStrategy.State} */
  this.state_ = remoting.SignalStrategy.State.NOT_CONNECTED;

  /** @type {!remoting.Error} */
  this.error_ = remoting.Error.none();

  this.onIncomingStanzaCallback_ = function() {};
};

/** @override */
remoting.MockSignalStrategy.prototype.dispose = function() {
};

/** @override */
remoting.MockSignalStrategy.prototype.connect = function() {
  var that = this;
  Promise.resolve().then(function() {
    that.setStateForTesting(remoting.SignalStrategy.State.CONNECTED);
  });
};

/** @override */
remoting.MockSignalStrategy.prototype.sendMessage = function() {
};

/**
 * @param {function(remoting.SignalStrategy.State):void} onStateChangedCallback
 *   Callback to call on state change.
 */
remoting.MockSignalStrategy.prototype.setStateChangedCallback = function(
    onStateChangedCallback) {
  this.onStateChangedCallback_ = onStateChangedCallback;
};

/**
 * @param {?function(Element):void} onIncomingStanzaCallback Callback to call on
 *     incoming messages.
 */
remoting.MockSignalStrategy.prototype.setIncomingStanzaCallback =
    function(onIncomingStanzaCallback) {
  this.onIncomingStanzaCallback_ =
      onIncomingStanzaCallback ? onIncomingStanzaCallback
                               : function() {};
};

/** @param {Element} stanza */
remoting.MockSignalStrategy.prototype.mock$onIncomingStanza = function(stanza) {
  this.onIncomingStanzaCallback_(stanza);
};

/** @return {remoting.SignalStrategy.State} */
remoting.MockSignalStrategy.prototype.getState = function() {
  return this.state_;
};

/** @return {!remoting.Error} */
remoting.MockSignalStrategy.prototype.getError = function() {
  return this.error_;
};

/** @return {string} */
remoting.MockSignalStrategy.prototype.getJid = function() {
  return this.jid_;
};

/** @return {remoting.SignalStrategy.Type} */
remoting.MockSignalStrategy.prototype.getType = function() {
  return this.type_;
};

/**
 * @param {remoting.SignalStrategy.State} state
 */
remoting.MockSignalStrategy.prototype.setStateForTesting = function(state) {
  this.state_ = state;
  if (state == remoting.SignalStrategy.State.FAILED) {
    this.error_ = remoting.Error.unexpected('setStateForTesting');
  } else {
    this.error_ = remoting.Error.none();
  }
  this.onStateChangedCallback_(state);
};
