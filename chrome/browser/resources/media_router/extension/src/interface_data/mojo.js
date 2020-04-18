// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Closure definitions of Mojo service objects.
 */

goog.provide('mr.MediaRouterRequestHandler');
goog.provide('mr.MediaRouterService');

goog.require('mr.RouteMessage');


/**
 * @interface
 *
 * Don't convert this interface to an ES6 class!  Doing so causes things to
 * break in strange ways.
 */
mr.MediaRouterService = function() {};



/**
 * @return {!Promise<!Object>}
 */
mr.MediaRouterService.getInstance = function() {
  if (!chrome.mojoPrivate || !chrome.mojoPrivate.requireAsync) {
    return Promise.reject(Error('No mojo service loaded'));
  }
  return new Promise((resolve, reject) => {
    // requireAsync does not return failures by design, so there is no error
    // handler here.
    chrome.mojoPrivate.requireAsync('media_router_bindings').then(mr => {
      const mediaRouter = /** @type {!mr.MediaRouterService} */ (mr);
      // Import all definitions into mojo namespace.

      mojo = mediaRouter.getMojoExports && mediaRouter.getMojoExports();
      mediaRouter.start().then(result => {
        resolve({
          'mrService': mediaRouter,

          'mrInstanceId': result['instance_id'] || result,
          'mrConfig': result['config']
        });
      });
    });
  });
};


/**
 * @param {!mr.MediaRouterRequestHandler} handlers
 * @export
 */
mr.MediaRouterService.prototype.setHandlers;


/**
 * Starts the MediaRouterService.
 * @return {!Promise<!{
 *     instance_id: string,
 *     config: !mojo.MediaRouteProviderConfig
 * }>} Resolved with an Object containing the result when MediaRouterService is
 *     started.
 * @export
 */
mr.MediaRouterService.prototype.start;


/**
 * Returns an Object containing Mojo definitions exported by
 * media_router_bindings.
 * @return {!Object}
 * @export
 */
mr.MediaRouterService.prototype.getMojoExports;


/**
 * @param {string} source
 * @param {!Array.<!mr.Sink>} sinks
 * @param {!Array<!mojo.Origin>} origins
 * @export
 */
mr.MediaRouterService.prototype.onSinksReceived;


/**
 * @param {string} pseudoSinkId
 * @param {string} sinkId
 * @export
 */
mr.MediaRouterService.prototype.onSearchSinkIdReceived;


/**
 * Used by Media Router Provider Manager to inform Media Router that there is an
 * issue.
 * @param {!mr.Issue} issue
 * @export
 */
mr.MediaRouterService.prototype.onIssue;


/**
 * Used by Media Router Provider Manager to inform Media Router that the list
 * of routes has been updated.
 * @param {Array.<mr.Route>} routes
 * @param {string=} opt_source
 * @param {Array.<string>=} opt_nonLocalJoinableRouteIds
 * @export
 */
mr.MediaRouterService.prototype.onRoutesUpdated;


/**
 * Used by Media Router Provider Manager to inform Media Router that sink
 * availability has changed.
 * @param {!mr.SinkAvailability} availability
 * @export
 */
mr.MediaRouterService.prototype.onSinkAvailabilityUpdated;


/**
 * Used by Media Router Provider Manager to inform Media Router that the state
 * of a presentation connected to a route has changed.
 * @param {string} routeId
 * @param {string} state
 * @export
 */
mr.MediaRouterService.prototype.onPresentationConnectionStateChanged;


/**
 * Used by Media Router Provider Manager to inform Media Router that the
 * presentation connected to a route has closed.
 * @param {string} routeId
 * @param {string} reason
 * @param {string} message
 * @export
 */
mr.MediaRouterService.prototype.onPresentationConnectionClosed;


/**
 * Informs Media Router of route messages received from the media sink to which
 * the route is connected.
 * @param {string} routeId
 * @param {!Array<!mr.RouteMessage>} messages
 * @export
 */
mr.MediaRouterService.prototype.onRouteMessagesReceived;


/**
 * Disables or enables extension event page suspension.
 * @param {boolean} keepAlive
 * @export
 */
mr.MediaRouterService.prototype.setKeepAlive;


/**
 * Gets the current keep alive state.
 * @return {boolean}
 * @export
 */
mr.MediaRouterService.prototype.getKeepAlive;


/**
 * Informs Media Router that a MirrorServiceRemoter is created for the given
 * tab. The Media Router may use remoterPtr to control media remoting. The Media
 * Router should also bind sourceRequest to an implementation to receive updates
 * on the remoting session.
 * @param {number} tabId
 * @param {!mojo.MirrorServiceRemoterPtr} remoterPtr
 * @param {!mojo.InterfaceRequest} sourceRequest
 * @export
 */
mr.MediaRouterService.prototype.onMediaRemoterCreated;


/**
 * Gets current status of media sink service from browser.
 * @return {!Promise<!{status: string}>}
 * @export
 */
mr.MediaRouterService.prototype.getMediaSinkServiceStatus;


/**
 * @interface
 */
mr.MediaRouterRequestHandler = function() {};


/**
 * Will be called immediately before any other handler method is invoked.
 * @export
 */
mr.MediaRouterRequestHandler.prototype.onBeforeInvokeHandler;


/**
 * Creates a route for |mediaSource| to |sinkId|.
 * @param {string} sourceUrn The URN of the media being displayed.
 * @param {string} sinkId
 * @param {string} presentationId A presentation ID to use.

 * @param {!mojo.Origin|string=} origin
 * @param {number=} tabId
 * @param {number=} timeoutMillis If positive, the timeout to use in place
 *     of default timeout.
 * @param {boolean=} offTheRecord If true, the request is from an off the
 *     record (incognito) browser profile.
 * @return {!Promise<!mr.Route>} Fulfilled with route created if successful.
 *     Rejected otherwise.
 * @export
 */
mr.MediaRouterRequestHandler.prototype.createRoute;


/**
 * Joins an existing route.
 *
 * @param {string} sourceUrn
 * @param {string} presentationId A presentation ID for Presentation API
 *  client; A Cast session ID for Cast join; and 'autojoin' for Cast auto Join
 *  case;
 * @param {!mojo.Origin|string} origin
 * @param {number} tabId
 * @param {number=} timeoutMillis If positive, the timeout to use in place
 *     of default timeout.
 * @param {boolean=} offTheRecord If true, the request is from an off the
 *     record (incognito) browser profile.
 * @return {!Promise<!mr.Route>} Fulfilled with route joined if successful.
 *     Rejected otherwise.
 * @export
 */
mr.MediaRouterRequestHandler.prototype.joinRoute;


/**
 * Joins an existing route by route Id.
 *
 * @param {string} sourceUrn
 * @param {string} routeId An existing route Id to join.
 * @param {string} presentationId The presentation Id of the route being
 *     created.
 * @param {!mojo.Origin|string} origin
 * @param {number} tabId
 * @param {number=} timeoutMillis If positive, the timeout to use in place
 *     of default timeout.
 * @return {!Promise<!mr.Route>} Fulfilled with route connected if successful.
 *     Rejected otherwise.
 * @export
 */
mr.MediaRouterRequestHandler.prototype.connectRouteByRouteId;


/**
 * Terminates the route specified by |routeId|.
 * @param {string} routeId The ID of the route to be terminated.
 * @return {!Promise<void>} Resolved if the route was terminated, rejected
 *     otherwise.
 * @export
 */
mr.MediaRouterRequestHandler.prototype.terminateRoute;


/**
 * Starts querying for sinks capable of displaying |sourceUrn|.
 * @param {string} sourceUrn The URN of the media.
 * @export
 */
mr.MediaRouterRequestHandler.prototype.startObservingMediaSinks;


/**
 * Stops querying for sinks capable of displaying |sourceUrn|.
 * @param {string} sourceUrn The URN of the media.
 * @export
 */
mr.MediaRouterRequestHandler.prototype.stopObservingMediaSinks;


/**
 * Sends a message to the sink via a media route.
 * @param {string} routeId
 * @param {!Object|string} message The message to post. Object will be
 *   serialized to a JSON string.
 * @param {Object=} opt_extraInfo Extra info about how to send a message.
 * @return {!Promise} Fulfilled when the message is posted, or rejected
 *   if there an error.
 * @export
 */
mr.MediaRouterRequestHandler.prototype.sendRouteMessage;


/**
 * Sends a binary message to the sink via a media route.
 * @param {string} routeId
 * @param {!Uint8Array} data The binary message to send.
 * @return {!Promise} Fulfilled when the message is sent, or rejected
 *   if there an error.
 * @export
 */
mr.MediaRouterRequestHandler.prototype.sendRouteBinaryMessage;


/**
 * Called when the MediaRouter wants to start receiving messages from the media
 * sink for the route identified by |routeId|.
 * @param {!string} routeId
 * @export
 */
mr.MediaRouterRequestHandler.prototype.startListeningForRouteMessages;


/**
 * Called when the MediaRouter wants to stop getting further messages
 * associated with the routeId.
 * @param {!string} routeId
 * @export
 */
mr.MediaRouterRequestHandler.prototype.stopListeningForRouteMessages;


/**
 * Informs the Media Router Provider Manager to send updates on routes list.
 * @param {string} sourceUrn The URN of the media.
 * @export
 */
mr.MediaRouterRequestHandler.prototype.startObservingMediaRoutes;


/**
 * Informs the Media Router Provider Manager to stop sending updates on routes
 * list.
 * @param {string} sourceUrn The URN of the media.
 * @export
 */
mr.MediaRouterRequestHandler.prototype.stopObservingMediaRoutes;


/**
 * Informs the Media Router Provider Manager that a presentation connection has
 * detached from its underlying media route due to garbage collection or
 * explicit close().
 * @param {!string} routeId
 * @export
 */
mr.MediaRouterRequestHandler.prototype.detachRoute;


/**
 * Enables mDNS discovery. No-ops if it is already enabled. Calling this will
 * trigger a firewall prompt on Windows if there is not already a firewall rule
 * for mDNS.
 * @export
 */
mr.MediaRouterRequestHandler.prototype.enableMdnsDiscovery;


/**
 * Searches the appropriate provider for a sink matching |searchCriteria| that
 * is compatible with |sourceUrn|. The provider that is searched is chosen to be
 * the one that owns the pseudo sink identified by |sinkId|. If any sinks are
 * found, sinks observers for |sourceUrn| will be invoked via onSinksReceived()
 * with those sinks included. The function will return the ID of a sink to which
 * a route can be created or the empty string if no such sink exists.
 *
 * Note that returning the empty string does not necessarily mean no matching
 * sinks were found. The provider could find multiple matching sinks and not
 * know how to choose a single one for the route creation. The MRPM will return
 * the sink on which the user is most likely to create the next route. However,
 * the manager does not enforce that the provider creates at most one sink in
 * response to a search.
 *
 * @param {string} sinkId Sink ID of the pseudo sink that generated the request.
 * @param {string} sourceUrn Source to be used with the sink.
 * @param {!mr.SinkSearchCriteria} searchCriteria Sink search criteria for the
 *     MRP's which includes the user's current domain.
 * @return {!Promise<string>} Fulfilled with the ID of a sink to which a route
 *     can be created. Rejected otherwise.
 * @export
 */
mr.MediaRouterRequestHandler.prototype.searchSinks;

/**
 * Called when Media Router finishes sink discovery.
 * @param {string} providerName Name of provider where the sinks come from.
 * @param {!Array<!mojo.Sink>} list of sinks discovered by Media Router.
 * @export
 */
mr.MediaRouterRequestHandler.prototype.provideSinks;

/**
 * Updates sinks even if sinkAvailability is UNAVAILABLE to allow for query
 * based discovery providers an opportuntity to find sinks.
 * @param {string} sourceUrn The URN of the media.
 * @export
 */
mr.MediaRouterRequestHandler.prototype.updateMediaSinks;


/**
 * Creates and returns the MediaRouteController instance for the given route.
 * Also sets the media status observer for the given route.
 * Rejects if the controller cannot be created, or if the controller
 * already exists.
 * @param {string} routeId
 * @param {!mojo.InterfaceRequest} controllerRequest The Mojo request object to
 *     be bound to the controller created.
 * @param {!mojo.MediaStatusObserverPtr} observer The observer's Mojo pointer.
 * @return {!Promise<void>}
 * @export
 */
mr.MediaRouterRequestHandler.prototype.createMediaRouteController;
