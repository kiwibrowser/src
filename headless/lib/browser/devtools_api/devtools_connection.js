// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Contains a class which marshals DevTools protocol messages over
 * a provided low level message transport.
 */

'use strict';

goog.provide('chromium.DevTools.Connection');

/**
 * Handles sending and receiving DevTools JSON protocol messages over the
 * provided low level message transport.
 * @export
 */
chromium.DevTools.Connection = class {
  /**
   * @param {!Object} transport The API providing transport for devtools
   *     commands.
   * @param {function(string, Object=): undefined =} opt_validator An
   *     optional function which performs checks before sending any DevTools
   *     messages.  It should throw an error if validation fails.
   */
  constructor(transport, opt_validator) {
    /** @private {!Object} */
    this.transport_ = transport;

    /** @private {function(string, Object=): undefined|undefined} */
    this.validator_ = opt_validator;

    /** @private {number} */
    this.commandId_ = 1;

    /**
     * An object containing pending DevTools protocol commands keyed by id.
     *
     * @private {!Map<number, !chromium.DevTools.Connection.PendingCommand>}
     */
    this.pendingCommands_ = new Map();

    /** @private {number} */
    this.nextListenerId_ = 1;

    /**
     * An object containing DevTools protocol events we are listening for keyed
     * by name.
     *
     * @private {!Map<string,
     *                !Map<number,
     *                     !chromium.DevTools.Connection.EventFunction>>}
     */
    this.eventListeners_ = new Map();

    /**
     * Used for removing event listeners by id.
     *
     * @private {!Map<number, string>}
     */
    this.eventListenerIdToEventName_ = new Map();

    /**
     * An object containing listeners for all DevTools protocol events keyed
     * by listener id.
     *
     * @private {!Map<number, !chromium.DevTools.Connection.AllEventsFunction>}
     */
    this.allEventsListeners_ = new Map();

    this.transport_.onmessage = this.onJsonMessage_.bind(this);
  }

  /**
   * Listens for DevTools protocol events of the specified name and issues the
   * callback upon reception.
   *
   * @param {string} eventName Name of the DevTools protocol event to listen
   *     for.
   * @param {!chromium.DevTools.Connection.EventFunction} listener The callback
   *     issued when we receive a DevTools protocol event corresponding to the
   *     given name.
   * @return {number} The id of this event listener.
   */
  addEventListener(eventName, listener) {
    if (!this.eventListeners_.has(eventName)) {
      this.eventListeners_.set(eventName, new Map());
    }
    let id = this.nextListenerId_++;
    this.eventListeners_.get(eventName).set(id, listener);
    this.eventListenerIdToEventName_.set(id, eventName);
    return id;
  }


  /**
   * Removes an event listener previously added by
   * <code>addEventListener</code>.
   *
   * @param {number} id The id of the event listener to remove.
   * @return {boolean} Whether the event listener was actually removed.
   */
  removeEventListener(id) {
    if (!this.eventListenerIdToEventName_.has(id)) return false;
    let eventName = this.eventListenerIdToEventName_.get(id);
    this.eventListenerIdToEventName_.delete(id);
    // This shouldn't happen, but lets check anyway.
    if (!this.eventListeners_.has(eventName)) return false;
    return this.eventListeners_.get(eventName).delete(id);
  }

  /**
   * Listens for all DevTools protocol events and issues the
   * callback upon reception.
   *
   * @param {!chromium.DevTools.Connection.AllEventsFunction} listener The
   *     callback issued when we receive a DevTools protocol event.
   * @return {number} The id of this event listener.
   */
  addAllEventsListener(listener) {
    let id = this.nextListenerId_++;
    this.allEventsListeners_.set(id, listener);
    return id;
  }

  /**
   * Removes an event listener previously added by
   * <code>addAllEventsListener</code>.
   *
   * @param {number} id The id of the event listener to remove.
   * @return {boolean} Whether the event listener was actually removed.
   */
  removeAllEventsListener(id) {
    if (!this.allEventsListeners_.has(id)) return false;
    return this.allEventsListeners_.delete(id);
  }

  /**
   * Issues a DevTools protocol command and returns a promise for the results.
   *
   * @param {string} method The name of the DevTools protocol command method.
   * @param {!Object=} params An object containing the command parameters if
   *     any.
   * @return {!Promise<!TYPE>} A promise for the results object.
   * @template TYPE
   */
  sendDevToolsMessage(method, params = {}) {
    let id = this.commandId_;
    // We increment by two because these bindings are intended to be used in
    // conjunction with HeadlessDevToolsClient::RawProtocolListener and using
    // odd numbers for js generated IDs lets the implementation of =
    // OnProtocolMessage easily distinguish between C++ and JS generated
    // commands and route the response accordingly.
    this.commandId_ += 2;
    if (this.validator_) {
      this.validator_(method, params);
    }
    // Note the names are in quotes to prevent closure compiler name mangling.
    this.transport_.send(
        JSON.stringify({'method': method, 'id': id, 'params': params}));
    return new Promise((resolve, reject) => {
      this.pendingCommands_.set(id, {resolve: resolve, reject: reject});
    });
  }

  /**
   * @param {string} jsonMessage A string containing a JSON DevTools protocol
   *     message.
   * @private
   */
  onJsonMessage_(jsonMessage) {
    let message = JSON.parse(jsonMessage);
    if (message.hasOwnProperty('id')) {
      if (!this.pendingCommands_.has(message.id))
        throw new Error('Unrecognized id:' + jsonMessage);
      if (message.hasOwnProperty('error'))
        this.pendingCommands_.get(message.id).reject(message.error);
      else
        this.pendingCommands_.get(message.id).resolve(message.result);
      this.pendingCommands_.delete(message.id);
    } else {
      if (!message.hasOwnProperty('method') ||
          !message.hasOwnProperty('params')) {
        throw new Error('Bad message:' + jsonMessage);
      }
      const method = message['method'];
      const params = message['params'];
      this.allEventsListeners_.forEach(function(listener) {
        listener({method, params});
      });
      if (this.eventListeners_.has(method)) {
        this.eventListeners_.get(method).forEach(function(listener) {
          listener(params);
        });
      }
    }
  }
}

/**
 * @typedef {function(Object): undefined|function(string): undefined}
 */
chromium.DevTools.Connection.EventFunction;

/**
 * @typedef {function(Object): undefined}
 */
chromium.DevTools.Connection.AllEventsFunction;

/**
 * @typedef {{
 *    resolve: function(!Object),
 *    reject: function(!Object)
 * }}
 */
chromium.DevTools.Connection.PendingCommand;
