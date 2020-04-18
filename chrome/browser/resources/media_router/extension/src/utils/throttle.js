// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.module('mr.Throttle');
goog.module.declareLegacyNamespace();


/**
 * Throttle will perform an action that is passed in no more than once
 * per interval (specified in milliseconds). If it gets multiple signals
 * to perform the action while it is waiting, it will only perform the action
 * once at the end of the interval.
 * @final
 * @template T
 */
class Throttle {
  /**
   * @param {function(this: T, ...?)} listener Function to callback when the
   *     action is triggered.
   * @param {number} interval Interval over which to throttle. The listener can
   *     only be called once per interval.
   * @param {T=} handler Object in whose scope to call the listener.
   */
  constructor(listener, interval, handler = undefined) {
    /** @private @const */
    this.listener_ = handler != null ? listener.bind(handler) : listener;

    /** @private @const */
    this.interval_ = interval;

    /** @private @const */
    this.callback_ = this.onTimer_.bind(this);

    /**
     * The last arguments passed into `fire`.
     * @private {!Array<?>}
     */
    this.args_ = [];

    /**
     * Indicates that the action is pending and needs to be fired.
     * @private {boolean}
     */
    this.shouldFire_ = false;

    /**
     * Timer for scheduling the next callback
     * @private {?number}
     */
    this.timerId_ = null;
  }

  /**
   * Notifies the throttle that the action has happened. It will throttle the
   * call so that the callback is not called too often according to the interval
   * parameter passed to the constructor, passing the arguments from the last
   * call of this function into the throttled function.
   * @param {...?} args Arguments to pass on to the throttled function.
   */
  fire(...args) {
    this.args_ = [...args];
    if (this.timerId_ == null) {
      this.doAction_();
    } else {
      this.shouldFire_ = true;
    }
  }

  /**
   * Calls the callback
   * @private
   */
  doAction_() {
    this.timerId_ = setTimeout(this.callback_, this.interval_);
    this.listener_(...this.args_);
  }

  /**
   * Handler for the timer to fire the throttle
   * @private
   */
  onTimer_() {
    this.timerId_ = null;

    if (this.shouldFire_) {
      this.shouldFire_ = false;
      this.doAction_();
    }
  }
}

exports = Throttle;
