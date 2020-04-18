// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview API for media route providers.
 *

 */

goog.provide('mr.Provider');
goog.provide('mr.ProviderName');

goog.require('mr.CancellablePromise');

/**
 * @enum {string}
 */
mr.ProviderName = {
  CAST: 'cast',
  DIAL: 'dial',
  CLOUD: 'cloud',
  TEST: 'test'
};



/**
 * @record
 */
mr.Provider = class {
  /**
   * Gets provider name.
   * @return {string}
   */
  getName() {}

  /**
   * Called to send a message to the sink via a media route.
   * @param {string} routeId
   * @param {!Object|string} message The message to send. Object will be
   *   serialized to a JSON string.
   * @param {Object=} opt_extraInfo Extra info about how to send a message.
   * @return {!Promise<void>} Fulfilled when the message is posted, or rejected
   *   if there an error.
   */
  sendRouteMessage(routeId, message, opt_extraInfo) {}

  /**
   * Called to send a binary message to the sink via a media route.
   * @param {string} routeId
   * @param {!Uint8Array} data The message to send.
   * @return {!Promise<void>} Fulfilled when the message is posted, or rejected
   *   if there an error.
   */
  sendRouteBinaryMessage(routeId, data) {}

  /**
   * Called the first time the provider is loaded.
   * Should do any one-time initialization (i.e. register event filters, etc.)
   * @param {!mojo.MediaRouteProviderConfig=} config The config object from the
   * browser to initialize the provider with.
   */
  initialize(config = undefined) {}

  /**
   * Queries this provider for existing routes.
   *
   * @return {!Array.<!mr.Route>}
   */
  getRoutes() {}

  /**
   * Queries this provider for available sinks.
   *
   * @param {string} sourceUrn
   * @return {!mr.SinkList}
   */
  getAvailableSinks(sourceUrn) {}

  /**
   * Starts querying for sinks capable of displaying |sourceUrn|.
   * @param {string} sourceUrn The URN of the media.
   */
  startObservingMediaSinks(sourceUrn) {}

  /**
   * Stops querying for sinks capable of displaying |sourceUrn|.
   * @param {string} sourceUrn The URN of the media.
   */
  stopObservingMediaSinks(sourceUrn) {}

  /**
   * Informs the provider to send updates on routes list.
   * @param {string} sourceUrn The URN of the media.
   */
  startObservingMediaRoutes(sourceUrn) {}

  /**
   * Informs the provider to stop sending updates on routes list.
   * @param {string} sourceUrn The URN of the media.
   */
  stopObservingMediaRoutes(sourceUrn) {}

  /**
   * Queries this provider for a sink by id.
   *
   * @param {string} sinkId
   * @return {?mr.Sink}
   *  Null if no sink exists with sinkId.
   */
  getSinkById(sinkId) {}

  /**
   * Creates a new route to the sink.
   * @param {string} sourceUrn
   * @param {string} sinkId The ID of the target sink.
   * @param {string} presentationId A presentation ID to use.
   * @param {boolean} offTheRecord True if the request is from an
   *     off the record (incognito) browser profile.
   * @param {number} timeoutMillis Request timeout in milliseconds.
   * @param {string=} opt_origin
   * @param {number=} opt_tabId
   * @return {!mr.CancellablePromise<!mr.Route>} Fulfilled with route created if
   *     successful. Rejected otherwise.
   */
  createRoute(
      sourceUrn, sinkId, presentationId, offTheRecord, timeoutMillis,
      opt_origin, opt_tabId) {}

  /**
   * Terminates the media route owned by this provider.
   * @param {string} routeId The media route id.
   * @return {!Promise} Fulfilled when route is terminated, or rejected with
   *     an error.
   */
  terminateRoute(routeId) {}

  /**
   * Creates and computes mirror settings appropriate for the given sink (and
   * the sender's capabilities). See class comments for mr.mirror.Settings when
   * overriding this method. Returns null if this provider does not support the
   * sink.
   *
   * The provider must return valid, frozen settings if
   * provider.canRoute(sourceUrn, sinkId) is true.
   *
   * @param {string} sinkId The ID of the sink to mirror to.
   * @return {!mr.mirror.Settings}
   */
  getMirrorSettings(sinkId) {}

  /**
   * Gets the name of the best mirror service supported by this provider
   * on the sink |sinkId|.
   *
   * Note that provider must return a valid mirror service name if
   * provider.canRoute(sourceUrn, sinkId) is true, where sourcerUrn is any
   * valid mirroring URN.
   *
   * @param {string} sinkId
   * @return {?mr.mirror.ServiceName}
   *  Null if no mirror service is supported on the sink.
   */
  getMirrorServiceName(sinkId) {}

  /**
   * Tells the provider that the mirroring activity description for the
   * mirroring route |routeId| has changed.  The provider can synchronize this
   * with its own state.
   * @param {string} routeId
   */
  onMirrorActivityUpdated(routeId) {}

  /**
   * Whether this provider can route media |sourceUrn| to sink |sinkId|.
   * @param {string} sourceUrn The URN of the media being displayed.
   * @param {string} sinkId
   * @return {boolean} True if the provider can handle it.
   */
  canRoute(sourceUrn, sinkId) {}

  /**
   * Whether this provider can join a given route from |sourceUrn| and,
   *  optionally, the specific |route| in question.
   * @param {string} sourceUrn The URN of the media being displayed.
   * @param {string=} presentationId The presentation ID to join.
   * @param {mr.Route=} route The route to join.
   * @return {boolean} True if the provider can handle it.
   */
  canJoin(sourceUrn, presentationId = undefined, route = undefined) {}

  /**
   * Joins a route identified by by |sourceUrn| and |presentationId|.
   * @param {string} sourceUrn
   * @param {string} presentationId A presentation ID for Presentation API
   *  client; A Cast session ID for Cast join; and 'autojoin' for Cast auto Join
   *  case;
   * @param {boolean} offTheRecord True if the request is from an
   *     off the record (incognito) browser profile.
   * @param {number} timeoutMillis Request timeout in milliseconds.
   * @param {string} origin
   * @param {?number} tabId null for packaged app.
   * @return {!mr.CancellablePromise<!mr.Route>} Fulfilled with the route if
   *     joined; Rejected otherwise.
   */
  joinRoute(
      sourceUrn, presentationId, offTheRecord, timeoutMillis, origin, tabId) {}

  /**
   * Joins a route identified by by |sourceUrn| and |routeId|.
   * @param {string} sourceUrn
   * @param {string} routeId A route ID to join.
   * @param {string} presentationId The presentation ID of the route to be
   *     created.
   * @param {string} origin
   * @param {?number} tabId null for packaged app.
   * @param {number=} opt_timeoutMillis If positive, the timeout to use in place
   *     of default timeout.
   * @return {!mr.CancellablePromise<!mr.Route>} Fulfilled with the route if
   *     joined; Rejected otherwise.
   */
  connectRouteByRouteId(
      sourceUrn, routeId, presentationId, origin, tabId, opt_timeoutMillis) {}

  /**
   * Detaches a presentation connection from the underlying media route given by
   * |routeId|.
   * @param {!string} routeId
   */
  detachRoute(routeId) {}

  /**
   * Searches this provider for a sink that matches |searchCriteria| that is
   * compatible with the source |sourceUrn|. Returns a promise which resolves
   * to the matching sink, or rejected if not found.
   * @param {string} sourceUrn
   * @param {!mr.SinkSearchCriteria} searchCriteria
   * @return {!Promise<!mr.Sink>}
   */
  searchSinks(sourceUrn, searchCriteria) {}

  /**
   * Called when Media Router finishes sink discovery. Store |sinks| in this
   * provider.
   * @param {!Array<!mojo.Sink>} sinks list of discovered sinks
   */
  provideSinks(sinks) {}

  /**
   * See documentation in interface_data/mojo.js.
   * @param {string} routeId
   * @param {!mojo.InterfaceRequest} controllerRequest
   * @param {!mojo.MediaStatusObserverPtr} observer
   * @return {!Promise<void>}
   */
  createMediaRouteController(routeId, controllerRequest, observer) {}
};
