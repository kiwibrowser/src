// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview A sender for sending route message to MR. When MR asks for
 *  the next batch of messages, this sender sends matching messages if
 *  available, or buffer the request till message arrives.
 *

 */

goog.provide('mr.RouteMessageSender');

goog.require('mr.Assertions');
goog.require('mr.Logger');
goog.require('mr.PersistentData');
goog.require('mr.PersistentDataManager');
goog.require('mr.RouteMessage');
goog.require('mr.ThrottlingSender');


/**
 * @implements {mr.PersistentData}
 */
mr.RouteMessageSender = class extends mr.ThrottlingSender {
  /**
   * @param {!mr.ProviderManagerCallbacks} providerManagerCallbacks
   * @param {number} messageSizeKeepAliveThreshold
   * @final
   */
  constructor(providerManagerCallbacks, messageSizeKeepAliveThreshold) {
    super(mr.RouteMessageSender.SEND_MESSAGE_INTERVAL_MILLIS);

    /**
     * Route ID to queue map. One queue per route.
     * @private {!Map<string, !Array<!mr.RouteMessage>>}
     */
    this.queues_ = new Map();

    /**
     * Set of routes being listened for messages.
     * @private {!Set<string>}
     */
    this.listeningRouteIds_ = new Set();

    /**
     * Callback to invoke to send a batch of route messages. Set during
     * |init()|. The first argument is the route id, and the second argument is
     * the batch of messages to send.
     * @private {?function(string, !Array<!mr.RouteMessage>)}
     */
    this.sendMessagesCallback_ = null;

    /**
     * Sum of sizes of all string messages currently queued up. When this is
     * above a certain threshold, the extension will be kept alive due to
     * performance reasons.
     * @private {number}
     */
    this.totalMessageSize_ = 0;

    /**
     * Number of enqueued binary messages. The extension will be kept alive
     * while there are enqueued binary messages.
     * @private {number}
     */
    this.binaryMessageCount_ = 0;

    /**
     * See |totalMessageSize_| and |binaryMessageCount_|.
     * @private {boolean}
     */
    this.shouldKeepAlive_ = false;

    /**
     * @private {!mr.ProviderManagerCallbacks}
     */
    this.providerManagerCallbacks_ = providerManagerCallbacks;

    /**
     * @private {number}
     */
    this.messageSizeKeepAliveThreshold_ = messageSizeKeepAliveThreshold;

    /** @private {mr.Logger} */
    this.logger_ = mr.Logger.getInstance('mr.RouteMessageSender');
  }

  /**
   * @param {!function(string, !Array<!mr.RouteMessage>)} sendMessagesCallback
   *     The callback to invoke to send a batch of route messages. See comments
   *     on |sendMessagesCallback_|.
   */
  init(sendMessagesCallback) {
    this.sendMessagesCallback_ = sendMessagesCallback;
    mr.PersistentDataManager.register(this);
  }

  /**
   * Starts listening for route messages associated with |routeId|, and schedule
   * a task to send any available messages to the Media Router.
   *
   * @param {string} routeId
   */
  listenForRouteMessages(routeId) {
    if (this.listeningRouteIds_.has(routeId)) {
      return;
    }

    this.listeningRouteIds_.add(routeId);
    if (this.hasMessageFrom_(routeId)) {
      this.scheduleSend();
    }
  }

  /**
   * The media router wants to stop getting further messages associated with the
   * routeId until it issues listenForRouteMessages again.
   *
   * @param {string} routeId
   */
  stopListeningForRouteMessages(routeId) {
    this.listeningRouteIds_.delete(routeId);
  }

  /**
   * Called when there is a |message| for |routeId| available to be sent.
   * @param {string} routeId
   * @param {string|!Uint8Array} message
   */
  send(routeId, message) {
    let queue = this.queues_.get(routeId);
    if (!queue) {
      queue = [];
      this.queues_.set(routeId, queue);
    }

    const routeMessage = new mr.RouteMessage(routeId, message);
    queue.push(routeMessage);

    // If the queue size for this route has grown suspiciously large, log
    // warnings as the queue size grows past the warning threshold.
    //

    if (queue.length > mr.RouteMessageSender.QUEUE_SIZE_WARNING_THRESHOLD_ &&
        queue.length % mr.RouteMessageSender.QUEUE_SIZE_WARNING_THRESHOLD_ ==
            1) {
      this.logger_.warning(
          () => `Message queue length is excessively large ` +
              `(${queue.length}) for route ${routeId}`);
    }

    this.totalMessageSize_ += mr.RouteMessage.stringLength(routeMessage);
    if (mr.RouteMessage.isBinary(routeMessage)) {
      this.binaryMessageCount_++;
    }

    this.updateShouldKeepAlive_();
    if (this.listeningRouteIds_.has(routeId)) {
      this.scheduleSend();
    }
  }

  /**
   * Removes queue on route removal.
   * @param {string} routeId
   */
  onRouteRemoved(routeId) {
    this.listeningRouteIds_.delete(routeId);
    const queue = this.queues_.get(routeId);
    if (queue) {
      this.queues_.delete(routeId);
      this.onMessagesRemoved_(queue);
      this.updateShouldKeepAlive_();
    }
  }

  /**
   * Update message size and binary message counters as |messages| are being
   * removed from the queue.
   * @param {!Array<!mr.RouteMessage>} messages
   * @private
   */
  onMessagesRemoved_(messages) {
    if (messages.length == 0) {
      return;
    }

    for (let message of messages) {
      this.totalMessageSize_ -= mr.RouteMessage.stringLength(message);
      if (mr.RouteMessage.isBinary(message)) {
        this.binaryMessageCount_--;
      }
    }
  }

  /**
   * @param {string} routeId
   * @return {boolean} True if there is at least one message from the route.
   * @private
   */
  hasMessageFrom_(routeId) {
    const queue = this.queues_.get(routeId);
    return !!queue && queue.length > 0;
  }

  /**
   * Computes whether the extension should be kept alive, and informs the
   * Provider Manager if that value changed.
   * @private
   */
  updateShouldKeepAlive_() {
    const newShouldKeepAlive = this.binaryMessageCount_ > 0 ||
        this.totalMessageSize_ > this.messageSizeKeepAliveThreshold_;
    if (newShouldKeepAlive != this.shouldKeepAlive_) {
      this.shouldKeepAlive_ = newShouldKeepAlive;
      this.providerManagerCallbacks_.requestKeepAlive(
          this.getStorageKey(), newShouldKeepAlive);
    }
  }

  /**
   * @override
   */
  doSend() {
    if (!this.sendMessagesCallback_) {
      this.logger_.error(
          'sendMessagesCallback not set. Messages not delivered.');
      return;
    }

    for (const routeId of this.listeningRouteIds_) {
      const queue = this.queues_.get(routeId);
      if (!queue || (queue.length == 0)) {
        continue;
      }
      this.sendMessagesCallback_(routeId, queue);
      this.onMessagesRemoved_(queue);
      this.queues_.set(routeId, []);
    }
    this.updateShouldKeepAlive_();
  }

  /**
   * @override
   */
  getStorageKey() {
    return 'mr.RouteMessageSender';
  }

  /**
   * @override
   */
  getData() {
    // Assumption: While there are binary messages in any queue, the extension
    // keep-alive should be turned on. Thus, we should not encounter binary
    // messages while persisting the queues here.
    const persistableQueues = [...this.queues_.entries()].map(entry => {
      return [
        entry[0],
        entry[1].map(
            message => mr.Assertions.assertString(
                message.message, 'No support for persisting binary messages'))
      ];
    });
    return [new mr.RouteMessageSender.PersistentData_(
        persistableQueues, Array.from(this.listeningRouteIds_),
        this.totalMessageSize_)];
  }

  /**
   * @override
   */
  loadSavedData() {
    const savedData = /** @type {?mr.RouteMessageSender.PersistentData_} */
        (mr.PersistentDataManager.getTemporaryData(this));
    if (savedData) {
      this.queues_ = new Map();
      for (const entry of savedData.queues) {
        const routeId = /** @type {string} */ (entry[0]);
        // Assumption: In getData(), there should not have been any binary
        // messages persisted. Therefore, only string messages should be
        // restored here.
        const queue = (/** @type {!Array<*>} */ (entry[1])).map(message => {
          return new mr.RouteMessage(
              routeId,
              mr.Assertions.assertString(
                  message, 'No support for restoring binary messages'));
        });
        this.queues_.set(routeId, queue);
      }
      this.listeningRouteIds_ = new Set(savedData.listeningRouteIds);
      this.totalMessageSize_ = savedData.totalMessageSize;
    }
  }
};


/**
 * The interval at which messages will be sent back to Media Router.
 * @const {number}
 */
mr.RouteMessageSender.SEND_MESSAGE_INTERVAL_MILLIS = 20;


/**
 * Generally, no route should have more than 50 messages in queue. However,
 * there may be momentary spikes for high-volume communications (e.g., RPC
 * traffic).
 * @private @const {number}
 */
mr.RouteMessageSender.QUEUE_SIZE_WARNING_THRESHOLD_ = 50;


/**
 * If the total number of characters in all enqueued string messages exceeds
 * this threshold, the extension will be kept alive for performance reasons.
 * @const {number}
 */
mr.RouteMessageSender.MESSAGE_SIZE_KEEP_ALIVE_THRESHOLD = 512 * 1024;


/**
 * @private
 */
mr.RouteMessageSender.PersistentData_ = class {
  /**
   * @param {!Array<Array<*>>} queues An array where each element is an
   * 2-element array of [routeId, messages].
   * @param {!Array<string>} listeningRouteIds
   * @param {number} totalMessageSize
   */
  constructor(queues, listeningRouteIds, totalMessageSize) {
    /** @type {!Array<Array<*>>} */
    this.queues = queues;

    /** @type {!Array<string>} */
    this.listeningRouteIds = listeningRouteIds;

    /** @type {number} */
    this.totalMessageSize = totalMessageSize;
  }
};
