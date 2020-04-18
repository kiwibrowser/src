// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * A class for attempting some unreliable operation until it succeeds.  Sort of
 * an asynchronous while loop.
 *
 *   (new mr.Retry(attempt, 500, 10)).start().then(onSuccess, onFailure);
 *
 * The next attempt is driven by the failure of current attempt. Thus, there is
 * at most one attempt invocation at any time.
 */

goog.provide('mr.Retry');

goog.require('mr.Assertions');
goog.require('mr.PromiseResolver');


/**
 * An object that attempts some operation until it succeeds.
 *
 * @template R
 */
mr.Retry = class {
  /**
   * @param {function():!Promise<R>} onAttempt An
   *     idempotent function to call repeatedly until it succeeds.
   * @param {number} retryDelay The number of milliseconds to wait between the
   *     start of one attempt and the start of the next. It must be positive.
   *     More precisely, this is the amount of time to wait between the failure
   *     of one call to onAttempt().
   * @param {number} maxAttempts The maximum number of attempts.
   *     It must be positive.
   */
  constructor(onAttempt, retryDelay, maxAttempts) {
    /**
     * @private {!function():!Promise<R>}
     */
    this.onAttempt_ = onAttempt;

    /**
     * @private {number}
     */
    this.retryDelay_ = retryDelay > 0 ? retryDelay : 10;

    /**
     * @private {number}
     */
    this.maxAttempts_ = maxAttempts > 0 ? maxAttempts : 1;

    /**
     * @private {number}
     */
    this.maxRetryDelay_ = 0;

    /**
     * @private {number}
     */
    this.backoffFactor_ = 1;

    /**
     * The number of times `onAttempt_` has been called.
     * @private {number}
     */
    this.numAttemptsStarted_ = 0;

    /**
     * @private {boolean}
     */
    this.isFinished_ = false;

    /**
     * The ID of the most recently created timer.
     * @private {?number}
     */
    this.timerId_ = null;

    /** @private {mr.PromiseResolver} */
    this.resolver_ = null;
  }

  /**
   * Starts running this object.
   *
   * This method starts an asynchronous process that repeatedly calls
   * `onAttempt`.
   *
   * For each attempt, `onAttempt` is called. When `onAttempt`
   * resolves, the returned promise is resolved with the same result.
   * The returned promise rejects if `abort` is called on this object,
   * or the number of attempts specified by `setMaxAttempts` is reached.
   *
   * @return {!Promise<R>}
   * @template R
   */
  start() {
    if (this.resolver_ != null) {
      return Promise.reject(Error('Cannot call Retry.start more than once.'));
    }
    this.resolver_ = new mr.PromiseResolver();
    this.retryOnce_();
    return this.resolver_.promise;
  }

  /**
   * Makes the next call to `onAttempt_`.
   * @private
   */
  retryOnce_() {
    this.timerId_ = null;
    if (this.isFinished_) {
      // The abort method has been called, don't start a new attempt.
      return;
    }

    this.numAttemptsStarted_++;
    this.onAttempt_().then(
        result => {
          this.cleanup_();
          this.resolver_.resolve(result);
        },
        error => {
          if (this.numAttemptsStarted_ >= this.maxAttempts_) {
            // Maximum number of attempts has been reached, do not try again.
            this.cleanup_();
            this.resolver_.reject(Error('Max attempts reached'));
          } else {
            this.timerId_ =
                setTimeout(this.retryOnce_.bind(this), this.retryDelay_);
            this.updateRetryDelay_();
          }
        });
  }

  /**
   * Implement exponential backoff.
   * @private
   */
  updateRetryDelay_() {
    let newRetryDelay = this.retryDelay_ * this.backoffFactor_;
    if (this.maxRetryDelay_ > 0) {
      newRetryDelay = Math.min(newRetryDelay, this.maxRetryDelay_);
    }
    this.retryDelay_ = newRetryDelay;
  }

  /**
   * Sets the backoff factor.  After each attempt, the retry delay is
   * multiplied by the backoff factor, which must be at least 1.
   *
   * @param {number} backoffFactor The factor by which the retry delay should be
   *     increased after each attempt.
   * @return {!mr.Retry} this
   */
  setBackoffFactor(backoffFactor) {
    mr.Assertions.assert(backoffFactor >= 1);
    this.backoffFactor_ = backoffFactor;
    return this;
  }

  /**
   * Sets the maximum retry delay that can be used as a result of the backoff
   * factor.  If 0, there is no maximum.
   * @param {number} maxRetryDelay The maximum number of milliseconds to wait
   *     before retrying.
   * @return {!mr.Retry} this
   */
  setMaxRetryDelay(maxRetryDelay) {
    mr.Assertions.assert(maxRetryDelay >= 0);
    this.maxRetryDelay_ = maxRetryDelay;
    return this;
  }

  /**
   * Causes this object to stop making attempts and puts it in a
   * finished state.  May be called any time after `start`.
   */
  abort() {
    this.cleanup_();
    this.resolver_.reject(Error('abort'));
  }

  /**
   * @private
   */
  cleanup_() {
    if (this.timerId_ != null) {
      // Clean up any timer, because it will be a no-op when it fires.
      clearTimeout(this.timerId_);
      this.timerId_ = null;
    }

    this.isFinished_ = true;
  }
};
