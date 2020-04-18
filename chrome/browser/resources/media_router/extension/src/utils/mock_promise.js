// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.provide('mr.MockPromise');
goog.setTestOnly('mr.MockPromise');


/**
 * Why does this class exist?
 *
 * Most of our unit tests that involve promises are written in "synchronous
 * style".  In this style, everything happens in one iteration of the JS event
 * loop. With native promises, this style isn't possible, because a call like
 * p.then(f) won't run f until at least the next event loop iteration, even if p
 * is already resolved.
 *
 * When the tests were originally written, they relied on a particular
 * interaction between goog.testing.MockClock and goog.Promise, where calling
 * mockClock.tick() would force any code scheduled by goog.Promise to be
 * executed immediately.  Since then, the non-test code has been changed to use
 * native promises, and the test code has been changed to use this class and
 * mr.MockClock, but the same principle applies.
 *
 * The long-term plan is to convert the tests to use asynchronous style, and get
 * rid of this class and mr.MockClock entirely.
 *
 * @template TYPE
 * @final
 */
mr.MockPromise = class {
  /**
   * @param {function(
   *             function((TYPE|mr.MockPromise<TYPE>)=),
   *             function(*=)): void} resolver
   */
  constructor(resolver) {
    /**
     * The internal state of this Promise. Either PENDING, FULFILLED, REJECTED,
     * or BLOCKED.
     * @private {mr.MockPromise.State_}
     */
    this.state_ = mr.MockPromise.State_.PENDING;

    /**
     * The settled result of the Promise. Immutable once set with either a
     * fulfillment value or rejection reason.
     * @private {*}
     */
    this.result_ = undefined;

    /**
     * The linked list of `onFulfilled` and `onRejected` callbacks
     * added to this Promise by calls to {@code then()}.
     * @private {!Array<!mr.MockPromise.CallbackEntry_>}
     */
    this.callbackEntries_ = [];

    /**
     * Whether the Promise is in the queue of Promises to execute.
     * @private {boolean}
     */
    this.executing_ = false;

    /**
     * A boolean that is set if the Promise is rejected, and reset to false if
     * an `onRejected` callback is invoked for the Promise (or one of its
     * descendants). If the rejection is not handled before the next timestep,
     * the rejection reason is passed to the unhandled rejection handler.
     * @private {boolean}
     */
    this.hadUnhandledRejection_ = false;

    try {
      resolver.call(
          null,
          value => {
            this.resolve_(mr.MockPromise.State_.FULFILLED, value);
          },
          reason => {
            try {
              // Promise was rejected. Step up one call frame to see why.
              if (reason instanceof Error) {
                throw reason;
              } else {
                throw new Error('Promise rejected.');
              }
            } catch (e) {
              // Only thrown so browser dev tools can catch rejections of
              // promises when the option to break on caught exceptions is
              // activated.
            }
            this.resolve_(mr.MockPromise.State_.REJECTED, reason);
          });
    } catch (e) {
      this.resolve_(mr.MockPromise.State_.REJECTED, e);
    }
  }

  /**
   * Replaces the native Promise class with this class.
   */
  static install() {
    if (window.Promise !== mr.MockPromise.origPromise_) {
      throw Error('Error installing mr.MockPromise');
    }
    if (mr.MockPromise.pendingHandlers_.length) {
      throw Error('Expected no pending handlers.');
    }
    window.Promise = mr.MockPromise;
    mr.MockPromise.ignoreUnhandledRejections = false;
  }

  /**
   * Undoes the effect of calling install().
   */
  static uninstall() {
    if (window.Promise !== mr.MockPromise) {
      throw Error('mr.MockPromise not installed');
    }
    if (mr.MockPromise.pendingHandlers_.length) {
      console.warn('Discarding pending handlers.');
      mr.MockPromise.pendingHandlers_.length = 0;
      // throw Error('Expected no pending handlers.');
    }
    window.Promise = mr.MockPromise.origPromise_;
  }

  /**
   * @return {void}
   */
  static callPendingHandlers() {
    while (mr.MockPromise.pendingHandlers_.length) {
      const handler = mr.MockPromise.pendingHandlers_.shift();
      handler();
    }
  }

  /**
   * @param {Function} onFulfilled
   * @param {Function} onRejected
   * @return {!mr.MockPromise.CallbackEntry_}
   * @private
   */
  static getCallbackEntry_(onFulfilled, onRejected) {
    const entry = new mr.MockPromise.CallbackEntry_();
    entry.onFulfilled = onFulfilled;
    entry.onRejected = onRejected;
    return entry;
  }

  /**
   * @param {*=} opt_value
   * @return {!mr.MockPromise} A new Promise that is immediately resolved
   *     with the given value. If the input value is already a mr.MockPromise,
   * it will be returned immediately without creating a new instance.
   */
  static resolve(opt_value) {
    if (opt_value instanceof mr.MockPromise) {
      // Avoid creating a new object if we already have a promise object
      // of the correct type.
      return opt_value;
    }

    return new mr.MockPromise((resolve, reject) => {
      resolve(opt_value);
    });
  }

  /**
   * @param {*=} opt_reason
   * @return {!mr.MockPromise} A new Promise that is immediately rejected with the
   *     given reason.
   */
  static reject(opt_reason) {
    return new mr.MockPromise((resolve, reject) => {
      reject(opt_reason);
    });
  }

  /**
   * @param {!Array} promises
   * @return {!mr.MockPromise} A Promise that receives the result of the first
   *     Promise input to settle immediately after it settles.
   */
  static race(promises) {
    return new mr.MockPromise((resolve, reject) => {
      if (!promises.length) {
        resolve(undefined);
      }
      for (let i = 0, promise; i < promises.length; i++) {
        promise = promises[i];
        mr.MockPromise.resolve(promise).then(resolve, reject);
      }
    });
  }

  /**
   * @param {!Array} promises
   * @return {!mr.MockPromise<!Array>} A Promise that receives a list of
   *     every fulfilled value once every input Promise is successfully
   *     fulfilled, or is rejected with the first rejection reason immediately
   *     after it is rejected.
   */
  static all(promises) {
    return new mr.MockPromise((resolve, reject) => {
      let toFulfill = promises.length;
      const values = [];

      if (!toFulfill) {
        resolve(values);
        return;
      }

      const onFulfill = (index, value) => {
        toFulfill--;
        values[index] = value;
        if (toFulfill == 0) {
          resolve(values);
        }
      };

      const onReject = reason => {
        reject(reason);
      };

      for (let i = 0, promise; i < promises.length; i++) {
        promise = promises[i];
        mr.MockPromise.resolve(promise).then(onFulfill.bind(null, i), onReject);
      }
    });
  }

  /**
   * Adds callbacks that will operate on the result of the Promise, returning a
   * new child Promise.
   *
   * If the Promise is fulfilled, the `onFulfilled` callback will be
   * invoked with the fulfillment value as argument, and the child Promise will
   * be fulfilled with the return value of the callback. If the callback throws
   * an exception, the child Promise will be rejected with the thrown value
   * instead.
   *
   * If the Promise is rejected, the `onRejected` callback will be invoked
   * with the rejection reason as argument, and the child Promise will be
   * resolved with the return value or rejected with the thrown value of the
   * callback.
   *
   * @param {?(function(TYPE):?)=} onFulfilled A
   *     function that will be invoked with the fulfillment value if the Promise
   *     is fulfilled.
   * @param {?(function(*): *)=} onRejected A function that will
   *     be invoked with the rejection reason if the Promise is rejected.
   * @return {!mr.MockPromise} A new Promise that will receive the result of the
   *     callback.
   * @override
   */
  then(onFulfilled = null, onRejected = null) {
    if (onFulfilled != null && typeof onFulfilled != 'function') {
      throw Error('onFulfilled should be a function.');
    }
    if (onRejected != null && typeof onRejected != 'function') {
      throw Error('onRejected should be a function.');
    }

    return this.addChildPromise_(onFulfilled, onRejected);
  }

  /**
   * Adds a callback that will be invoked only if the Promise is rejected. This
   * is equivalent to {@code then(null, onRejected)}.
   *
   * @param {function(*): *} onRejected A function that will be
   *     invoked with the rejection reason if the Promise is rejected.
   * @return {!mr.MockPromise} A new Promise that will receive the result of the
   *     callback.
   */
  catch(onRejected) {
    return this.addChildPromise_(null, onRejected);
  }

  /**
   * Adds a callback entry to the current Promise, and schedules callback
   * execution if the Promise has already been settled.
   *
   * @param {mr.MockPromise.CallbackEntry_} callbackEntry Record containing
   *     {
   * @private
   */
  addCallbackEntry_(callbackEntry) {
    if (!this.hasEntry_() &&
        (this.state_ == mr.MockPromise.State_.FULFILLED ||
         this.state_ == mr.MockPromise.State_.REJECTED)) {
      this.scheduleCallbacks_();
    }
    this.queueEntry_(callbackEntry);
  }

  /**
   * Creates a child Promise and adds it to the callback entry list. The result
   * of the child Promise is determined by the result of the `onFulfilled`
   * or `onRejected` callbacks as specified in the Promise resolution
   * procedure.
   *
   * @param {?function(TYPE):
   *          (RESULT|mr.MockPromise<RESULT>)} onFulfilled A callback that
   *     will be invoked if the Promise is fulfilled, or null.
   * @param {?function(*): *} onRejected A callback that will be
   *     invoked if the Promise is rejected, or null.
   * @return {!mr.MockPromise} The child Promise.
   * @template RESULT
   * @private
   */
  addChildPromise_(onFulfilled, onRejected) {
    /** @type {mr.MockPromise.CallbackEntry_} */
    const callbackEntry = mr.MockPromise.getCallbackEntry_(null, null, null);

    callbackEntry.child = new mr.MockPromise((resolve, reject) => {
      // Invoke onFulfilled, or resolve with the parent's value if absent.
      callbackEntry.onFulfilled = onFulfilled ? value => {
        try {
          const result = onFulfilled.call(null, value);
          resolve(result);
        } catch (err) {
          reject(err);
        }
      } : resolve;

      // Invoke onRejected, or reject with the parent's reason if absent.
      callbackEntry.onRejected = onRejected ? reason => {
        try {
          resolve(onRejected.call(null, reason));
        } catch (err) {
          reject(err);
        }
      } : reject;
    });

    this.addCallbackEntry_(callbackEntry);
    return callbackEntry.child;
  }

  /**
   * Unblocks the Promise and fulfills it with the given value.
   *
   * @param {TYPE} value
   * @private
   */
  unblockAndFulfill_(value) {
    if (this.state_ != mr.MockPromise.State_.BLOCKED) {
      throw Error('Expected state to be BLOCKED.');
    }
    this.state_ = mr.MockPromise.State_.PENDING;
    this.resolve_(mr.MockPromise.State_.FULFILLED, value);
  }

  /**
   * Unblocks the Promise and rejects it with the given rejection reason.
   *
   * @param {*} reason
   * @private
   */
  unblockAndReject_(reason) {
    if (this.state_ != mr.MockPromise.State_.BLOCKED) {
      throw Error('Expected state to be BLOCKED.');
    }
    this.state_ = mr.MockPromise.State_.PENDING;
    this.resolve_(mr.MockPromise.State_.REJECTED, reason);
  }

  /**
   * Attempts to resolve a Promise with a given resolution state and value. This
   * is a no-op if the given Promise has already been resolved.
   *
   * If the given result is a Promise, the Promise will be settled with the same
   * state and result as the Thenable once it is itself settled.
   *
   * If the given result is not a Promise, the Promise will be settled
   * (fulfilled or rejected) with that result based on the given state.
   *
   * @param {mr.MockPromise.State_} state
   * @param {*} x The result to apply to the Promise.
   * @private
   */
  resolve_(state, x) {
    if (this.state_ != mr.MockPromise.State_.PENDING) {
      return;
    }

    if (this === x) {
      state = mr.MockPromise.State_.REJECTED;
      x = new TypeError('Promise cannot resolve to itself');
    }

    this.state_ = mr.MockPromise.State_.BLOCKED;

    if (x instanceof mr.MockPromise) {
      x.addCallbackEntry_(mr.MockPromise.getCallbackEntry_(
          this.unblockAndFulfill_.bind(this),
          this.unblockAndReject_.bind(this)));
      return;
    }

    this.result_ = x;
    this.state_ = state;
    this.scheduleCallbacks_();

    if (state == mr.MockPromise.State_.REJECTED) {
      mr.MockPromise.addUnhandledRejection_(this, x);
    }
  }

  /**
   * Executes the pending callbacks of a settled Promise after a timeout.
   * @private
   */
  scheduleCallbacks_() {
    if (!this.executing_) {
      this.executing_ = true;
      mr.MockPromise.pendingHandlers_.push(this.executeCallbacks_.bind(this));
    }
  }

  /**
   * @return {boolean} Whether there are any pending callbacks queued.
   * @private
   */
  hasEntry_() {
    return this.callbackEntries_.size > 0;
  }

  /**
   * @param {mr.MockPromise.CallbackEntry_} entry
   * @private
   */
  queueEntry_(entry) {
    if (entry.onFulfilled == null) {
      throw Error('entry.onFulfilled == null');
    }
    this.callbackEntries_.push(entry);
  }

  /**
   * @return {mr.MockPromise.CallbackEntry_} entry
   * @private
   */
  popEntry_() {
    const entry = this.callbackEntries_.shift() || null;
    if (entry && entry.onFulfilled == null) {
      throw Error('entry.onFulfulled == null');
    }
    return entry;
  }

  /**
   * Executes all pending callbacks for this Promise.
   *
   * @private
   */
  executeCallbacks_() {
    let entry = null;
    while (entry = this.popEntry_()) {
      this.executeCallback_(entry, this.state_, this.result_);
    }
    this.executing_ = false;
  }

  /**
   * Executes a pending callback for this Promise. Invokes an
   * `onFulfilled` or `onRejected` callback based on the settled
   * state of the Promise.
   *
   * @param {!mr.MockPromise.CallbackEntry_} callbackEntry An entry containing
   *     the onFulfilled and/or onRejected callbacks for this step.
   * @param {mr.MockPromise.State_} state The resolution status of the Promise,
   *     either FULFILLED or REJECTED.
   * @param {*} result The settled result of the Promise.
   * @private
   */
  executeCallback_(callbackEntry, state, result) {
    // Cancel an unhandled rejection if the then call had an onRejected.
    if (state == mr.MockPromise.State_.REJECTED && callbackEntry.onRejected) {
      this.hadUnhandledRejection_ = false;
    }

    if (callbackEntry.child) {
      mr.MockPromise.invokeCallback_(callbackEntry, state, result);
    } else {
      try {
        mr.MockPromise.invokeCallback_(callbackEntry, state, result);
      } catch (err) {
        mr.MockPromise.handleRejection_(err);
      }
    }
  }

  /**
   * Executes the onFulfilled or onRejected callback for a callbackEntry.
   *
   * @param {!mr.MockPromise.CallbackEntry_} callbackEntry
   * @param {mr.MockPromise.State_} state
   * @param {*} result
   * @private
   */
  static invokeCallback_(callbackEntry, state, result) {
    if (state == mr.MockPromise.State_.FULFILLED) {
      callbackEntry.onFulfilled.call(null, result);
    } else if (callbackEntry.onRejected) {
      callbackEntry.onRejected.call(null, result);
    }
  }

  /**
   * Marks this rejected Promise as unhandled. If no `onRejected` callback
   * is called for this Promise before the `UNHANDLED_REJECTION_DELAY`
   * expires, the reason will be passed to the unhandled rejection handler. The
   * handler typically rethrows the rejection reason so that it becomes visible
   * in
   * the developer console.
   *
   * @param {!mr.MockPromise} promise The rejected Promise.
   * @param {*} reason The Promise rejection reason.
   * @private
   */
  static addUnhandledRejection_(promise, reason) {
    promise.hadUnhandledRejection_ = true;
    mr.MockPromise.pendingHandlers_.push(() => {
      if (promise.hadUnhandledRejection_) {
        mr.MockPromise.handleRejection_(reason);
      }
    });
  }

  /**
   * @param {*} reason
   * @private
   */
  static handleRejection_(reason) {
    if (!mr.MockPromise.ignoreUnhandledRejections) {
      throw reason;
    }
  }
};


/**
 * @type {boolean}
 */
mr.MockPromise.ignoreUnhandledRejections = false;



/**
 * @private @const
 */
mr.MockPromise.origPromise_ = Promise;


/**
 * The possible internal states for a Promise. These states are not directly
 * observable to external callers.
 * @enum {number}
 * @private
 */
mr.MockPromise.State_ = {
  /** The Promise is waiting for resolution. */
  PENDING: 0,

  /** The Promise is blocked waiting for the result of another Thenable. */
  BLOCKED: 1,

  /** The Promise has been resolved with a fulfillment value. */
  FULFILLED: 2,

  /** The Promise has been resolved with a rejection reason. */
  REJECTED: 3
};


/**
 * @private @const {!Array<function()>}
 */
mr.MockPromise.pendingHandlers_ = [];


/**
 * Entries in the callback chain. Each call to `then` or
 * `catch` creates an entry containing the
 * functions that may be invoked once the Promise is settled.
 *
 * @private @final
 */
mr.MockPromise.CallbackEntry_ = class {
  constructor() {
    /** @type {?mr.MockPromise} */
    this.child = null;
    /** @type {Function} */
    this.onFulfilled = null;
    /** @type {Function} */
    this.onRejected = null;
  }
};
