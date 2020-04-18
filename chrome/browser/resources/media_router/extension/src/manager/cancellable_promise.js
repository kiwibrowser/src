// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.module('mr.CancellablePromise');
goog.module.declareLegacyNamespace();


/**
 * A promise packaged together with a cancel() method.
 *
 * To understand cancellation, for consider a chain of ordinary Promise objects
 * P, Q, and R, where Q is the result of calling P.then, and R is the result of
 * calling Q.then.  Whenever Q is rejected, the rejection is propagated "down"
 * the chain to R, but P is unaffected.
 *
 * Now consider a chain of CancellablePromise objects P, Q, and R, created using
 * the chain() method below.  Rejection behaves the same as with ordinary
 * Promise objects, but whenever Q is cancelled, the cancellation is propagated
 * "up" the chain to P.  Because a cancelled promise is also rejected, calling
 * Q.cancel() also causes the rejection to be propagated to R.  In this way,
 * cancellation propagates both up and down a chain of related promises.
 *
 * For an explanation of how this class is used in practice (in the interaction
 * between ProviderManager and RouteProvider classes), see the diagram in
 * <route-creation-timeout.svg.gz>.
 *
 * @template T
 */
class CancellablePromise {
  /**
   * @param {function(function(T), function(*))} init Function
   *     called immediatiately with resolve and reject functions passed as
   *     arguments.
   * @param {?function(*)=} onCancelled Optional function to be called when this
   *     CancellablePromise is cancelled.
   */
  constructor(init, onCancelled = null) {
    /**
     * @private {function(*)}
     */
    this.reject_;

    /**
     * @private {?function(*)}
     */
    this.onCancelled_ = onCancelled;

    /**
     * @const
     */
    this.promise = new Promise((resolve, reject) => {
      const resolve1 = value => {
        this.onCancelled_ = null;
        resolve(value);
      };
      const reject1 = reason => {
        this.onCancelled_ = null;
        reject(reason);
      };
      this.reject_ = reject1;
      init(resolve1, reject1);
    });
  }

  /**
   * Cancels this promise.
   *
   * Does nothing if |this.promise| is already settled.  Otherwise, causes
   * |this.promise| to be rejected with |reason|, and causes the |onHandled|
   * function passed to this class's constructor to be called with |reason|.
   *
   * @param {*} reason
   */
  cancel(reason) {
    this.reject_(reason);
    if (this.onCancelled_) {
      const onCancelled = this.onCancelled_;
      this.onCancelled_ = null;  // ensure cancel() method is idempotent
      setTimeout(() => onCancelled(reason), 0);
    }
  }

  /**
   * Chains CancellablePromises together like the then() method of ordinary
   * promises, with one extra feature: if the "child" promise returned by this
   * method is cancelled, then this promise is automatically cancelled as well.
   *
   * @param {?function(T):U} onResolved Function called when this promise is
   *     resolved.  The argument is the value with which this promise was
   *     resolved.  If the function returns, the return value is used to resolve
   *     the child promise.  If the function throws an error, the child promise
   *     is rejected with that error.  If this argument is null, it is
   *     equivalent to passing a function that returns its argument.
   * @param {?function(*):U=} onRejected Function called when this promise is
   *     rejected.  The argument is the reason with which this promise was
   *     rejected.  If the function returns, the return value is used to resolve
   *     the child promise.  If the function throws an error, the child promise
   *     is rejected with that error.  If this argument is missing or null, it
   *     is equivalent to passing a function that throws its argument.
   * @return {!CancellablePromise<U>} A child promise which is resolved or
   *     rejected depending on the result of calling |onResolved| or
   *     |onRejected|.
   * @template U
   */
  chain(onResolved, onRejected = null) {
    return new CancellablePromise(
        (resolve, reject) => {
          this.promise.then(
              value => {
                if (onResolved) {
                  try {
                    resolve(onResolved(value));
                  } catch (reason) {
                    reject(reason);
                  }
                } else {
                  resolve(value);
                }
              },
              reason => {
                if (onRejected) {
                  try {
                    resolve(onRejected(reason));
                  } catch (reason2) {
                    reject(reason2);
                  }
                } else {
                  reject(reason);
                }
              });
        },
        reason => {
          this.cancel(reason);
        });
  }

  /**
   * Make it Promise by expose then.
   * @param {?function(T):U} onResolved Function called when this promise is
   *     resolved.  The argument is the value with which this promise was
   *     resolved.  If the function returns, the return value is used to resolve
   *     the child promise.  If the function throws an error, the child promise
   *     is rejected with that error.  If this argument is null, it is
   *     equivalent to passing a function that returns its argument.
   * @param {?function(*):U=} onRejected Function called when this promise is
   *     rejected.  The argument is the reason with which this promise was
   *     rejected.  If the function returns, the return value is used to resolve
   *     the child promise.  If the function throws an error, the child promise
   *     is rejected with that error.  If this argument is missing or null, it
   *     is equivalent to passing a function that throws its argument.
   * @return {!CancellablePromise<U>} A child promise which is resolved or
   *     rejected depending on the result of calling |onResolved| or
   *     |onRejected|.
   * @template U
   */
  then(onResolved, onRejected = null) {
    return this.chain(onResolved, onRejected);
  }

  /**
   * Shorthand for .chain(null, onRejected).
   * @param {?function(*):T} onRejected
   * @return {!CancellablePromise<T>}
   */
  catch(onRejected) {
    return this.chain(null, onRejected);
  }

  /**
   * Utility function create a promise in a resolved state.
   * @param {T} value
   * @return {!CancellablePromise<T>}
   * @template T
   */
  static resolve(value) {
    return new CancellablePromise((resolve, reject) => {
      resolve(value);
    });
  }

  /**
   * Utility function create a promise in a rejected state.
   * @param {*} reason
   * @return {!CancellablePromise<T>}
   * @template T
   */
  static reject(reason) {
    return new CancellablePromise((resolve, reject) => {
      reject(reason);
    });
  }

  /**
   * Utility function to wrap a Promise.
   *

   *
   * @param {!Promise<T>} promise
   * @return {!CancellablePromise<T>}
   * @template T
   */
  static forPromise(promise) {
    return new CancellablePromise((resolve, reject) => {
      promise.then(resolve, reject);
    });
  }

  /**
   * Produces a CancellablePromise |outer| that runs the following steps:
   *
   * In the normal case, |outer| waits for a regular (non-cancellable) Promise
   * |promise| to resolve to |value|.  Then it calls |startCancellableStep|,
   * passing |value| as the argument, to produce a CancellablePromise |inner|.
   * When |inner| is settled, the result is used to settle |outer|.
   *
   * If |outer| is cancelled before |promise| is resolved, then |value| is
   * discarded and |startCancellableStep| is not called.
   *
   * If |outer| is cancelled after |promise| is resolved, then |inner| is
   * cancelled as well.
   *
   * If |promise| is rejected, then |outer| is rejected as well.
   *
   * @param {!Promise<A>} promise
   * @param {function(A):!CancellablePromise<B>} startCancellableStep
   * @return {!CancellablePromise<B>}
   * @template A, B
   */
  static withUncancellableStep(promise, startCancellableStep) {
    /** @type {boolean} */
    let wasCancelled = false;
    /** @type {CancellablePromise} */
    let innerPromise = null;

    return new CancellablePromise(
        (resolve, reject) => {
          promise.then(value => {
            if (!wasCancelled) {
              innerPromise = startCancellableStep(value);
              innerPromise.promise.then(resolve, reject);
            }
          }, reject);
        },
        reason => {
          if (innerPromise) {
            innerPromise.cancel(reason);
          } else {
            wasCancelled = true;
          }
        });
  }
}

exports = CancellablePromise;
