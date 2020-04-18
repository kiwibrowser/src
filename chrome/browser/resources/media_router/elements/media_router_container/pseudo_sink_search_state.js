// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * This class holds state that is relevant to the search process from the UI's
 * perspective. It primarily handles the spinner logic while waiting for a
 * search to complete. The spinner first needs to start on the pseudo sink, but
 * when a real sink arrives to replace it the spinner should transfer to the
 * real sink.
 *
 * Additionally, this class provides a method for
 * onCreateRouteResponseReceived() that maps the pseudo sink ID that started the
 * search to the real sink that was produced by the search. This helps check
 * whether a received route is valid.
 *
 * @param {!media_router.Sink} pseudoSink Pseudo sink that started the search.
 * @constructor
 */
var PseudoSinkSearchState = function(pseudoSink) {
  /**
   * Pseudo sink that started the search.
   * @private {!media_router.Sink}
   */
  this.pseudoSink_ = pseudoSink;

  /**
   * The ID of the sink that is found by search.
   * @private {string}
   */
  this.realSinkId_ = '';

  /**
   * Whether we have received a sink in the sink list with ID |realSinkId_|.
   * @private {boolean}
   */
  this.hasRealSink_ = false;
};

/**
 * Record the real sink ID returned from the Media Router.
 * @param {string} sinkId Real sink ID that is the result of the search.
 */
PseudoSinkSearchState.prototype.receiveSinkResponse = function(sinkId) {
  this.realSinkId_ = sinkId;
};

/**
 * Checks whether we have a sink in |sinkList| that is our search result then
 * computes the value for |currentLaunchingSinkId_| based on the state of the
 * search. It should be the pseudo sink ID until the real sink arrives, then the
 * real sink ID.
 * @param {!Array<!media_router.Sink>} sinkList List of all sinks to check.
 * @return {string} New value for |currentLaunchingSinkId_|.
 */
PseudoSinkSearchState.prototype.checkForRealSink = function(sinkList) {
  if (!this.hasRealSink_) {
    this.hasRealSink_ = !!this.realSinkId_ && sinkList.some(function(sink) {
      return (sink.id == this.realSinkId_);
    }, this);
    return !this.hasRealSink_ ? this.pseudoSink_.id : this.realSinkId_;
  }
  return this.realSinkId_;
};

/**
 * Returns the pseudo sink for the current search. This is used to enforce
 * freezing its name in filter view and displaying it in the sink list view.
 * @return {!media_router.Sink}
 */
PseudoSinkSearchState.prototype.getPseudoSink = function() {
  return this.pseudoSink_;
};
