// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Scheduler for requests. Fetches requests from a queue and processes them
 * synchronously, taking into account priorities. The highest priority is 0.
 * @constructor
 */
function Scheduler() {
  /**
   * List of requests waiting to be checked. If these items are available in
   * cache, then they are processed immediately after starting the scheduler.
   * However, if they have to be downloaded, then these requests are moved
   * to pendingRequests_.
   *
   * @type {Array<ImageRequest>}
   * @private
   */
  this.newRequests_ = [];

  /**
   * List of pending requests for images to be downloaded.
   * @type {Array<ImageRequest>}
   * @private
   */
  this.pendingRequests_ = [];

  /**
   * List of requests being processed.
   * @type {Array<ImageRequest>}
   * @private
   */
  this.activeRequests_ = [];

  /**
   * Hash array of requests being added to the queue, but not finalized yet.
   * @type {Object}
   * @private
   */
  this.requests_ = {};

  /**
   * If the scheduler has been started.
   * @type {boolean}
   * @private
   */
  this.started_ = false;
}

/**
 * Maximum download requests to be run in parallel.
 * @type {number}
 * @const
 */
Scheduler.MAXIMUM_IN_PARALLEL = 5;

/**
 * Adds a request to the internal priority queue and executes it when requests
 * with higher priorities are finished. If the result is cached, then it is
 * processed immediately once the scheduler is started.
 *
 * @param {ImageRequest} request Request object.
 */
Scheduler.prototype.add = function(request) {
  if (!this.started_) {
    this.newRequests_.push(request);
    this.requests_[request.getId()] = request;
    return;
  }

  // Enqueue the request, since already started.
  this.pendingRequests_.push(request);
  this.sortPendingRequests_();

  this.continue_();
};

/**
 * Removes a request from the scheduler (if exists).
 * @param {string} requestId Unique ID of the request.
 */
Scheduler.prototype.remove = function(requestId) {
  var request = this.requests_[requestId];
  if (!request)
    return;

  // Remove from the internal queues with pending tasks.
  var newIndex = this.pendingRequests_.indexOf(request);
  if (newIndex != -1)
    this.newRequests_.splice(newIndex, 1);
  var pendingIndex = this.pendingRequests_.indexOf(request);
  if (pendingIndex != -1)
    this.pendingRequests_.splice(pendingIndex, 1);

  // Cancel the request.
  request.cancel();
  delete this.requests_[requestId];
};

/**
 * Starts handling requests.
 */
Scheduler.prototype.start = function() {
  this.started_ = true;

  // Process tasks added before scheduler has been started.
  this.pendingRequests_ = this.newRequests_;
  this.sortPendingRequests_();
  this.newRequests_ = [];

  // Start serving enqueued requests.
  this.continue_();
};

/**
 * Sorts pending requests by priorities.
 * @private
 */
Scheduler.prototype.sortPendingRequests_ = function() {
  this.pendingRequests_.sort(function(a, b) {
    return a.getPriority() - b.getPriority();
  });
};

/**
 * Processes pending requests from the queue. There is no guarantee that
 * all of the tasks will be processed at once.
 *
 * @private
 */
Scheduler.prototype.continue_ = function() {
  // Run only up to MAXIMUM_IN_PARALLEL in the same time.
  while (this.pendingRequests_.length &&
         this.activeRequests_.length < Scheduler.MAXIMUM_IN_PARALLEL) {
    var request = this.pendingRequests_.shift();
    this.activeRequests_.push(request);

    // Try to load from cache. If doesn't exist, then download.
    request.loadFromCacheAndProcess(
        this.finish_.bind(this, request),
        function(currentRequest) {
          currentRequest.downloadAndProcess(
              this.finish_.bind(this, currentRequest));
        }.bind(this, request));
  }
};

/**
 * Handles finished requests.
 *
 * @param {ImageRequest} request Finished request.
 * @private
 */
Scheduler.prototype.finish_ = function(request) {
  var index = this.activeRequests_.indexOf(request);
  if (index < 0)
    console.warn('Request not found.');
  this.activeRequests_.splice(index, 1);
  delete this.requests_[request.getId()];

  // Continue handling the most important requests (if started).
  if (this.started_)
    this.continue_();
};
