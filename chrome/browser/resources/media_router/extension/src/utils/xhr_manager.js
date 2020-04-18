// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.module('mr.XhrManager');
goog.module.declareLegacyNamespace();

const Assertions = goog.require('mr.Assertions');
const PromiseResolver = goog.require('mr.PromiseResolver');

/**
 * Wraps XmlHttpRequest API with additional functionalities such as queueing,
 * timeout, and retries.
 */
class XhrManager {
  /**
   * @param {number} maxRequests The maximum number of concurrent XHR
   *     requests.
   * @param {number} defaultTimeoutMillis The default timeout for each XHR
   *     request.
   * @param {number} defaultNumAttempts The default number of attempts for each
   *     operation.
   */
  constructor(maxRequests, defaultTimeoutMillis, defaultNumAttempts) {
    /** @private @const {number} */
    this.maxRequests_ = maxRequests;

    /** @private @const {number} */
    this.defaultTimeoutMillis_ = defaultTimeoutMillis;

    /** @private @const {number} */
    this.defaultNumAttempts_ = defaultNumAttempts;

    /**
     * Number of pending requests. Does not exceed this.maxRequests_.
     * @private {number}
     */
    this.numPendingRequests_ = 0;

    /**
     * Holds requests that have not yet been executed.
     * @private {!Array<!QueueEntry_>}
     */
    this.queuedRequests_ = [];
  }

  /**
   * Adds a request with the given parameters.
   * @param {string} url
   * @param {string} method
   * @param {string=} body
   * @param {{timeoutMillis: (number|undefined),
   *         numAttempts: (number|undefined),
   *         headers: (!Array<!Array<string>>|undefined),
   *         responseType: (string|undefined)}=} overrides "headers" is an Array
   *     of pairs of strings. "responseType" is a valid
   *     XMLHttpRequest.responseType enum value.
   * @return {!Promise<!XMLHttpRequest>} Resolves with the response. Rejects if
   *     the request timed out on all attempts.
   */
  send(url, method, body = undefined, {
    timeoutMillis = this.defaultTimeoutMillis_,
    numAttempts = this.defaultNumAttempts_,
    headers = null,
    responseType = '',
  } = {}) {
    const entry = {
      resolver: new PromiseResolver(),
      url: url,
      method: method,
      headers: headers,
      responseType: responseType,
      body: body,
      timeoutMillis: timeoutMillis,
      numAttemptsLeft: numAttempts
    };

    if (this.numPendingRequests_ < this.maxRequests_) {
      this.startRequest_(entry);
    } else {
      this.queuedRequests_.push(entry);
    }

    return entry.resolver.promise;
  }

  /**
   * Starts a request from the request queue if there is room for an additional
   * pending request.
   * @private
   */
  startNextRequestFromQueue_() {
    if (this.queuedRequests_.length > 0 &&
        this.numPendingRequests_ < this.maxRequests_) {
      const request = this.queuedRequests_.shift();
      this.startRequest_(request);
    }
  }

  /**
   * Attempts the given request once. If successful, resolves the
   * PromiseResolver with the result. Otherwise, requeues the request for retry,
   * or rejects the PromiseResolver if it ran out of attempts. Also processes a
   * queued request, if any, at the end.
   * @param {!QueueEntry_} request
   * @private
   */
  startRequest_(request) {
    this.numPendingRequests_++;
    Assertions.assert(
        request.numAttemptsLeft > 0, 'request.numAttemptsLeft > 0');
    request.numAttemptsLeft--;

    const cleanUpAndStartNextRequest = () => {
      this.numPendingRequests_--;
      this.startNextRequestFromQueue_();
    };

    this.sendOneAttempt_(request).then(
        response => {
          request.resolver.resolve(response);
          cleanUpAndStartNextRequest();
        },
        e => {
          if (request.numAttemptsLeft == 0) {
            request.resolver.reject(e);
          } else {
            // Try it again later by re-adding the request to back of queue.

            this.queuedRequests_.push(request);
          }
          cleanUpAndStartNextRequest();
        });
  }

  /**
   * Executes a XMLHttpRequest and returns a Promise that resolves with the
   * response, or rejects if the request times out.
   * @param {!QueueEntry_} request
   * @return {!Promise<!XMLHttpRequest>} Resolves with the response. Rejects if
   *     the request timed out.
   * @private
   */
  sendOneAttempt_(request) {
    return new Promise((resolve, reject) => {
      const xhr = new XMLHttpRequest();
      xhr.onreadystatechange = () => {
        if (xhr.readyState == XMLHttpRequest.DONE) {
          resolve(xhr);
        }
      };

      xhr.timeout = request.timeoutMillis;
      xhr.ontimeout = () => {
        reject(new Error('Timed out'));
      };

      xhr.open(request.method, request.url, true);
      if (request.headers == null) {
        xhr.setRequestHeader(
            'Content-Type', 'application/x-www-form-urlencoded;charset=utf-8');
      } else {
        request.headers.forEach(
            header => xhr.setRequestHeader(header[0], header[1]));
      }
      xhr.responseType = request.responseType;
      xhr.send(request.body);
    });
  }
}

/** @private @record */
const QueueEntry_ = class {};
/** @type {!PromiseResolver<!XMLHttpRequest>} */
QueueEntry_.prototype.resolver;
/** @type {string} */
QueueEntry_.prototype.url;
/** @type {string} */
QueueEntry_.prototype.method;
/** @type {?Array<!Array<string>>} */
QueueEntry_.prototype.headers;
/** @type {string} */
QueueEntry_.prototype.responseType;
/** @type {string|undefined} */
QueueEntry_.prototype.body;
/** @type {number} */
QueueEntry_.prototype.timeoutMillis;
/** @type {number} */
QueueEntry_.prototype.numAttemptsLeft;

exports = XhrManager;
