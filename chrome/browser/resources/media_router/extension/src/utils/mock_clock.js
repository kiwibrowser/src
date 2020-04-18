// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.setTestOnly('mr.MockClock');
goog.provide('mr.MockClock');

goog.require('mr.MockPromise');


/**
 * Class for unit testing code that uses setTimeout, clearTimeout, etc.
 * @final
 */
mr.MockClock = class {
  /**
   * Installs the MockClock by overriding the global object's
   * implementation of setTimeout, setInterval, clearTimeout and
   * clearInterval.
   */
  constructor() {
    if (window.setTimeout !== mr.MockClock.REAL_SETTIMEOUT_) {
      throw Error('MockClock already installed.');
    }

    /**
     * List of times to fire, sorted in reverse order of when they
     * will be executed.
     *
     * @type {!Array<mr.MockClock.Timeout_>}
     * @private
     */
    this.queue_ = [];

    /**
     * The current simulated time in milliseconds.
     * @type {number}
     * @private
     */
    this.nowMillis_ = 0;

    mr.MockClock.installedHere_ = Error('MockClock was installed here.');

    window.setTimeout = this.setTimeout_.bind(this);
    window.setInterval = this.setInterval_.bind(this);
    window.setImmediate = this.setImmediate_.bind(this);
    window.clearTimeout = this.clearTimeout_.bind(this);
    window.clearInterval = this.clearTimeout_.bind(this);
    Date.now = this.getCurrentTime_.bind(this);
  }

  /**
   * Removes the MockClock's hooks into the global object's functions
   * and revert to their original values.
   */
  uninstall() {
    if (window.setTimeout === mr.MockClock.REAL_SETTIMEOUT_) {
      throw Error('MockClock not installed.');
    }

    mr.MockClock.installedHere_ = null;

    window.setTimeout = mr.MockClock.REAL_SETTIMEOUT_;
    window.setInterval = mr.MockClock.REAL_SETINTERVAL_;
    window.setImmediate = mr.MockClock.REAL_SETIMMEDIATE_;
    window.clearTimeout = mr.MockClock.REAL_CLEARTIMEOUT_;
    window.clearInterval = mr.MockClock.REAL_CLEARINTERVAL_;
    Date.now = mr.MockClock.REAL_DATENOW_;
  }

  /**
   * Restores this clock to the state it was in just after it was
   * created.
   */
  reset() {
    this.queue_ = [];
    this.nowMillis_ = 0;
  }

  /**
   * Increments the MockClock's time by a given number of
   * milliseconds, running any functions that are now overdue.
   * @param {number=} millis Number of milliseconds to increment the
   *     counter.  If not specified, clock ticks 1 millisecond.
   * @return {number} Current mock time in milliseconds.
   */
  tick(millis = 1) {
    const endTime = this.nowMillis_ + millis;
    this.runFunctionsWithinRange_(endTime);
    this.nowMillis_ = endTime;
    return endTime;
  }

  /**
   * Ticks the clock until there are no more actions scheduled to run.
   */
  flush() {
    this.tick(Infinity);
  }

  /**
   * Takes a promise and then ticks the mock clock. If the promise
   * successfully resolves, returns the value produced by the
   * promise. If the promise is rejected, it throws the rejection as
   * an exception. If the promise is not resolved at all, throws an
   * exception.  Also ticks the general clock by the specified amount.
   *
   * @param {!mr.MockPromise<T>} promise A promise that should be
   *     resolved after the mockClock is ticked for the given
   *     opt_millis.
   * @param {number=} millis Number of milliseconds to increment the
   *     counter.  If not specified, clock ticks 1 millisecond.
   * @return {T}
   * @template T
   */
  tickPromise(promise, millis = 1) {
    let value;
    let error;
    let resolved = false;
    promise.then(
        v => {
          value = v;
          resolved = true;
        },
        e => {
          error = e;
          resolved = true;
        });
    this.tick(millis);
    if (!resolved) {
      throw new Error(
          'Promise was expected to be resolved ' +
          'after mock clock tick.');
    }
    if (error) {
      throw error;
    }
    return value;
  }

  /**
   * Takes a promise and then ticks the mock clock. If the promise
   * rejects, returns the error produced by the promise. If the
   * promise is rejected, it throws the rejection as an exception. If
   * the promise is not rejected at all, throws an exception.  Also
   * ticks the general clock by the specified amount.
   *
   * @param {!mr.MockPromise<T>} promise A promise that should be
   *     rejected after the mockClock is ticked for the given
   *     opt_millis.
   * @param {number=} millis Number of milliseconds to increment the
   *     counter.  If not specified, clock ticks 1 millisecond.
   * @return {*} Error produced by the promise.
   * @template T
   */
  tickRejectingPromise(promise, millis = 1) {
    let error;
    let rejected = false;
    promise.catch(e => {
      error = e;
      rejected = true;
    });
    this.tick(millis);
    if (!rejected) {
      throw new Error(
          'Promise was expected to be rejected after mock clock tick.');
    }
    return error;
  }

  /**
   * @return {number} The MockClock's current time in milliseconds.
   * @private
   */
  getCurrentTime_() {
    return this.nowMillis_;
  }

  /**
   * Runs any function that is scheduled before a certain time.
   * @param {number} endTime The latest time in the range, in
   *     milliseconds.
   * @private
   */
  runFunctionsWithinRange_(endTime) {
    mr.MockPromise.callPendingHandlers();

    // Repeatedly pop off the last item since the queue is always
    // sorted.
    while (this.queue_ && this.queue_.length &&
           this.queue_[this.queue_.length - 1].runAtMillis <= endTime) {
      const timeout = this.queue_.pop();
      // Only move time forwards.
      this.nowMillis_ = Math.max(this.nowMillis_, timeout.runAtMillis);
      if (timeout.recurring) {
        // Reschedule before calling the function so that if the
        // function deletes the timeout, it's in the queue to be
        // removed.
        this.scheduleFunction_(
            timeout.timeoutKey, timeout.funcToCall, timeout.millis, true);
      }
      timeout.funcToCall.call(undefined);
      mr.MockPromise.callPendingHandlers();
    }
  }

  /**
   * Schedules a function to be run at a certain time.
   * @param {number} timeoutKey The timeout key.
   * @param {!Function} funcToCall The function to call.
   * @param {number} millis The number of milliseconds to call it in.
   * @param {boolean} recurring Whether to function call should recur.
   * @private
   */
  scheduleFunction_(timeoutKey, funcToCall, millis, recurring) {
    const timeout = {
      runAtMillis: this.nowMillis_ + millis,
      funcToCall: funcToCall,
      recurring: recurring,
      timeoutKey: timeoutKey,
      millis: millis,
    };

    // Insert a timer descriptor into a descending-order queue.
    //
    // Later-inserted duplicates appear at lower indices.  For
    // example, the asterisk in (5,4,*,3,2,1) would be the insertion
    // point for 3.  (The numbers here refer to timestamps.)
    //
    // Insertion of N items is quadratic, but unit tests are normally
    // small, so scalability is not a primary issue.
    //
    // Since the queue is in reverse order (so we can pop rather than
    // unshift), and later timers with the same time stamp should be
    // executed later, we look for the element strictly greater than
    // the one we are inserting.
    let i;
    for (i = this.queue_.length; i != 0; i--) {
      if (this.queue_[i - 1].runAtMillis > timeout.runAtMillis) {
        break;
      }
      this.queue_[i] = this.queue_[i - 1];
    }
    this.queue_[i] = timeout;
  }

  /**
   * Schedules a function to be called after `millis`
   * milliseconds.  Mock implementation for setTimeout.
   * @param {!Function} funcToCall The function to call.
   * @param {number=} millis The number of milliseconds to call it
   *     after.
   * @param {...*} args Arguments to pass to the function.
   * @return {number} The number of timeouts created.
   * @private
   */
  setTimeout_(funcToCall, millis = 0, ...args) {
    if (millis > mr.MockClock.MAX_INT_) {
      throw Error(`Bad timeout value: ${millis}`);
    }
    this.scheduleFunction_(
        mr.MockClock.nextId_, funcToCall.bind(undefined, ...args), millis,
        false);
    return mr.MockClock.nextId_++;
  }

  /**
   * Schedules a function to be called every `millis` milliseconds.
   * Mock implementation for setInterval.
   * @param {!Function} funcToCall The function to call.
   * @param {number=} millis The number of milliseconds between calls.
   * @param {...*} args Arguments to pass to the function.
   * @return {number} The number of timeouts created.
   * @private
   */
  setInterval_(funcToCall, millis = 0, ...args) {
    this.scheduleFunction_(
        mr.MockClock.nextId_, funcToCall.bind(undefined, ...args), millis,
        true);
    return mr.MockClock.nextId_++;
  }

  /**
   * Schedules a function to be called immediately after the current JS
   * execution.
   * Mock implementation for setImmediate.
   * @param {!Function} funcToCall The function to call.
   * @param {...*} args Arguments to pass to the function.
   * @return {number} The number of timeouts created.
   * @private
   */
  setImmediate_(funcToCall, ...args) {
    return this.setTimeout_(funcToCall, 0, ...args);
  }

  /**
   * Clears a timeout.
   * Mock implementation for clearTimeout and clearInterval.
   * @param {number} timeoutKey The timeout key to clear.
   * @private
   */
  clearTimeout_(timeoutKey) {
    const newQueue =
        this.queue_.filter(timeout => timeout.timeoutKey != timeoutKey);
    if (newQueue.length == this.queue_.length_) {
      // The real versions of clearTimeout and clearInterval silently
      // ignore invalid keys, but we hold ourselves to a higher
      // standard :-)
      throw Error('Invalid timeoutKey');
    }
    this.queue_ = newQueue;
  }
};


/**
 * ID to use for next timeout.  Timeout IDs must never be reused, even
 * across MockClock instances.
 * @private {number}
 */
mr.MockClock.nextId_ = 0;


/**
 * @private @const
 */
mr.MockClock.REAL_SETTIMEOUT_ = window.setTimeout;


/**
 * @private @const
 */
mr.MockClock.REAL_SETINTERVAL_ = window.setInterval;


/**
 * @private @const
 */
mr.MockClock.REAL_SETIMMEDIATE_ = window.setImmediate;


/**
 * @private @const
 */
mr.MockClock.REAL_CLEARTIMEOUT_ = window.clearTimeout;


/**
 * @private @const
 */
mr.MockClock.REAL_CLEARINTERVAL_ = window.clearInterval;


/**
 * @private @const
 */
mr.MockClock.REAL_DATENOW_ = Date.now;


/**
 * Maximum 32-bit signed integer.
 *
 * Timeouts over this time return immediately in many browsers, due to
 * integer overflow.  Such known browsers include Firefox, Chrome, and
 * Safari, but not IE.
 *
 * @type {number}
 * @private
 */
mr.MockClock.MAX_INT_ = 2147483647;


/**
 * @typedef {{
 *   runAtMillis: number,
 *   funcToCall: !Function,
 *   recurring: boolean,
 *   timeoutKey: number,
 *   millis: number,
 * }}
 * @private
 */
mr.MockClock.Timeout_;


/**
 * Exception used to record where the current MockClock was created.  Helpful
 * for diagnosing unit tests that fail to uninstall their mock clocks.
 * @private {Error}
 */
mr.MockClock.installedHere_ = null;
