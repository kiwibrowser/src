// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.module('mr.EventTarget');
goog.module.declareLegacyNamespace();


/** @final */
class EventTarget {
  constructor() {
    /** @private @const {!Array<!Subscription>} */
    this.subscriptions_ = [];
  }

  /**
   * @param {string} type The event type id.
   * @param {function(this:T, ?)} handler Callback
   * @param {T=} target
   * @template T
   */
  listen(type, handler, target = undefined) {
    this.subscriptions_.push({type, handler, target});
  }

  /**
   * @param {string} type The event type id.
   * @param {function(this:T, ?)} handler Callback
   * @param {T=} target
   * @template T
   */
  unlisten(type, handler, target = undefined) {
    const index = this.subscriptions_.findIndex(
        sub =>
            sub.type == type && sub.handler == handler && sub.target == target);
    if (index != -1) {
      this.subscriptions_.splice(index, 1);
    }
  }

  /**
   * @param {{type: string}} event
   */
  dispatchEvent(event) {
    this.subscriptions_.forEach(sub => {
      if (sub.type == event.type) {
        // Call handler asynchronously so exceptions don't show up at the source
        // of the event.
        Promise.resolve().then(() => sub.handler.call(sub.target, event));
      }
    });
  }
}


/** @record */
const Subscription = class {};

/** @type {string} */
Subscription.prototype.type;

/** @type {function(?)} */
Subscription.prototype.handler;

/** @type {Object|undefined} */
Subscription.prototype.target;


exports = EventTarget;
