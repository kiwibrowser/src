// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview API for registering Media Route Providers.
 *

 *
 * For now, we will always load every provider each time the extension is
 * loaded.
 */

goog.provide('mr.ProviderManager');

goog.require('mr.Assertions');
goog.require('mr.CancellablePromise');
goog.require('mr.EventAnalytics');
goog.require('mr.EventTarget');
goog.require('mr.InitHelper');
goog.require('mr.InternalMessageEvent');
goog.require('mr.Logger');
goog.require('mr.MediaRouterRequestHandler');
goog.require('mr.MessagePortService');
goog.require('mr.MessagePortServiceImpl');
goog.require('mr.MirrorAnalytics');
goog.require('mr.Module');
goog.require('mr.MojoUtils');
goog.require('mr.PersistentData');
goog.require('mr.PersistentDataManager');
goog.require('mr.ProviderManagerCallbacks');
goog.require('mr.RouteMessageSender');
goog.require('mr.RouteRequestError');
goog.require('mr.RouteRequestResultCode');
goog.require('mr.Sink');
goog.require('mr.SinkAvailability');
goog.require('mr.SinkSearchCriteria');
goog.require('mr.Throttle');
goog.require('mr.mirror.Error');
goog.require('mr.mirror.ServiceName');


/**
 * Tracks registered MediaRouteProviders and loads them on-demand.
 * @implements {mr.MediaRouterRequestHandler}
 * @implements {mr.PersistentData}
 * @implements {mr.ProviderManagerCallbacks}
 */
mr.ProviderManager = class extends mr.Module {
  constructor() {
    super();

    /**
     * @private {mr.Logger}
     */
    this.logger_ = mr.Logger.getInstance('mr.ProviderManager');

    /**
     * Holds a an array of registered Media Route Providers.
     * @private {!Array<!mr.Provider>}
     */
    this.providers_ = [];

    /**
     * Holds a map of route ID to the provider that is managing the route.
     * @private {!Map<string, mr.Provider>}
     */
    this.routeIdToProvider_ = new Map();

    /**
     * Holds a map of route ID to the mirror service name if the route is for
     * tab/desktop mirroring.
     * @private {!Map<string, mr.mirror.ServiceName>}
     */
    this.routeIdToMirrorServiceName_ = new Map();

    /**
     * Holds a set of active sink queries.
     * @private {!Set<string>}
     */
    this.sinkQueries_ = new Set();

    /**
     * Holds a set of active route queries.
     * @private {!Set<string>}
     */
    this.routeQueries_ = new Set();

    /**
     * Holds a map of mirror service names to modules.
     * @private {!Map<mr.mirror.ServiceName, mr.ModuleId>}
     */
    this.mirrorServiceModules_ = new Map();

    /**
     * Keeps track of last used mirror service for gathering logs for feedback.
     * @private {?mr.mirror.ServiceName}
     */
    this.lastUsedMirrorService_ = null;

    /** @private {?mr.MediaRouterService} */
    this.mediaRouterService_ = null;

    /** @private {!mr.EventTarget} */
    this.routeMessageEventTarget_ = new mr.EventTarget();


    /** @private {!mr.Throttle} */
    this.routeUpdateEventThrottle_ = new mr.Throttle(
        this.sendRoutesQueryResultToMr_,
        mr.ProviderManager.ALL_QUERIES_INTERVAL_MS_, this);

    /** @private {!mr.Throttle} */
    this.sinkUpdateEventThrottle_ = new mr.Throttle(
        this.executeSinkQueries_, mr.ProviderManager.ALL_QUERIES_INTERVAL_MS_,
        this);

    /**
     * MR message sender with build-in rate throttle.
     * @private {!mr.RouteMessageSender}
     */
    this.mrRouteMessageSender_ = new mr.RouteMessageSender(
        this, mr.RouteMessageSender.MESSAGE_SIZE_KEEP_ALIVE_THRESHOLD);

    /**
     * The number of pending createRoute
     * @private {number}
     */
    this.pendingRequestRoutes_ = 0;

    /**
     * Maps provider name to that provider's last reported sink availability.
     * @private {!Map<string, mr.SinkAvailability>}
     */
    this.sinkAvailabilityMap_ = new Map();

    /**
     * Whether mDNS is currently enabled.
     * @private {boolean}
     */
    this.mdnsEnabled_ = window.navigator.userAgent.indexOf('Windows') == -1;

    /**
     * Functions that should be executed when |mdnsEnabled_| goes from |false|
     * to
     * |true|.
     * @private {!Array<function()>}
     */
    this.mdnsEnabledCallbacks_ = [];

    /**
     * Names of components that have requested to keep the extension alive.
     * @private {!Array<string>}
     */
    this.keepAliveComponents_ = [];

    /**
     * Handler for chrome.runtime.onMessage events.
     * @private @const
     */
    this.internalMessageHandler_ =
        mr.InitHelper.getInternalMessageHandler(this);

    /**
     * Handler for chrome.runtime.onMessageExternal events.
     * @private @const
     */
    this.externalMessageHandler_ =
        mr.InitHelper.getExternalMessageHandler(this);

    mr.ProviderManager.exportProperties_(this);
  }

  /**
   * @override
   */
  handleEvent(event, ...args) {
    if (event == chrome.runtime.onMessage) {
      this.internalMessageHandler_(...args);
    } else if (event == chrome.runtime.onMessageExternal) {
      this.externalMessageHandler_(...args);
    } else {
      throw new Error('Unhandled event');
    }
  }

  /**
   * Gets the mirror service with the given name.
   * @param {mr.mirror.ServiceName} serviceName Name of the mirror service.
   * @return {!Promise<!mr.mirror.Service>} Resolved with the requested
   *     mirror service.
   */
  getMirrorService(serviceName) {
    const moduleId = this.mirrorServiceModules_.get(serviceName);
    return mr.Module.load(moduleId).then(module => {
      let service = /** @type {!mr.mirror.Service} */ (module);
      service.initialize(this);
      return service;
    });
  }

  /**
   * Registers and initalizes providers.
   *
   * @param {!Array<!mr.Provider>} providers
   * @param {!mojo.MediaRouteProviderConfig=} config
   */
  registerAllProviders(providers, config = undefined) {
    providers.forEach(provider => {
      this.registerProvider_(provider, config);
    });
  }

  /**
   * Initializes the provider manager, register / initialize given providers,
   * and registers itself with PersistentDataManager.
   * @param {!mr.MediaRouterService} mediaRouterService
   * @param {!Array<!mr.Provider>} providers
   * @param {!mojo.MediaRouteProviderConfig=} config
   */
  initialize(mediaRouterService, providers, config = undefined) {
    this.mirrorServiceModules_.set(
        mr.mirror.ServiceName.WEBRTC, mr.ModuleId.WEBRTC_STREAMING_SERVICE);
    this.mirrorServiceModules_.set(
        mr.mirror.ServiceName.CAST_STREAMING,
        mr.ModuleId.CAST_STREAMING_SERVICE);
    this.mirrorServiceModules_.set(
        mr.mirror.ServiceName.HANGOUTS, mr.ModuleId.HANGOUTS_SERVICE);
    this.mirrorServiceModules_.set(
        mr.mirror.ServiceName.MEETINGS, mr.ModuleId.MEETINGS_SERVICE);

    mr.MessagePortService.setService(new mr.MessagePortServiceImpl(this));

    this.mediaRouterService_ = mediaRouterService;
    this.mrRouteMessageSender_.init(
        this.mediaRouterService_.onRouteMessagesReceived.bind(
            this.mediaRouterService_));
    this.registerAllProviders(providers, config);

    mr.PersistentDataManager.register(this);
    mr.Module.onModuleLoaded(mr.ModuleId.PROVIDER_MANAGER, this);
  }

  /**
   * Registers a provider and initializes it.
   * @param {!mr.Provider} provider
   * @param {!mojo.MediaRouteProviderConfig=} config
   * @private
   */
  registerProvider_(provider, config = undefined) {
    if (this.getProviderByName(provider.getName())) {
      this.logger_.warning(
          'Provider ' + provider.getName() + ' already registered.');
      return;
    }

    try {
      provider.initialize(config);
      this.providers_.push(provider);
      this.sinkAvailabilityMap_.set(
          provider.getName(), mr.SinkAvailability.UNAVAILABLE);
    } catch (/** Error */ error) {
      this.logger_.warning(
          'Provider ' + provider.getName() + ' failed to initialize.', error);
    }
  }

  /**
   * @return {!Array<!mr.Provider>} Registered Media Route Providers.
   */
  getProviders() {
    return this.providers_;
  }

  /**
   * @param {!mr.CancellablePromise<!mr.Route>} routePromise
   * @param {number} timeout Timeout in milliseconds. When timeout
   *    fires, the return value of this method is rejected.  Must be a positive
   *    number.
   * @return {!Promise<!mr.Route>}
   * @private
   */
  addTimeout_(routePromise, timeout) {
    return new Promise((resolve, reject) => {
      let timerId = null;
      this.preventSuspend_();
      timerId = window.setTimeout(() => {
        timerId = null;
        // Refer to the CancellablePromise class for more details on the
        // cancel() method, and <route-creation-timeout.svg.gz> for a diagram of
        // how promise cancellation works with this class.
        routePromise.cancel(new mr.RouteRequestError(
            mr.RouteRequestResultCode.TIMED_OUT,
            'timeout after ' + timeout + ' ms.'));
      }, timeout);
      const cleanup = () => {
        this.allowSuspend_();
        if (timerId != null) {
          window.clearTimeout(timerId);
        }
      };
      routePromise.promise.then(
          route => {
            cleanup();
            resolve(route);
          },
          err => {
            cleanup();
            reject(mr.RouteRequestError.wrap(err));
          });
    });
  }

  /**
   * @param {!Promise<T>} promise
   * @param {number} timeout Timeout in milliseconds. When timeout
   *    fires, the return value of this method is rejected.  Must be a positive
   *    number.
   * @return {!Promise<T>}
   * @template T
   * @private
   */
  addIgnoredTimeout_(promise, timeout) {
    return this.addTimeout_(mr.CancellablePromise.forPromise(promise), timeout);
  }

  /**
   * Increments the number of pending request for routes and checks if the
   * extension should be kept alive.
   * @private
   */
  preventSuspend_() {
    this.pendingRequestRoutes_++;
    this.maybeUpdateKeepAlive_();
  }

  /**
   * Decrements the number of pending request for routes and checks if the
   * extension should be kept alive.
   * @private
   */
  allowSuspend_() {
    this.pendingRequestRoutes_--;
    this.maybeUpdateKeepAlive_();
  }

  /**
   * If override timeout is provided and is positive, returns it.
   * Otherwise, returns the default timeout.
   * @param {number} defaultTimeoutMillis Default timeout.
   * @param {number=} opt_overrideTimeoutMillis Optional override timeout.
   * @return {number}
   * @private
   */
  static getTimeoutToUse_(defaultTimeoutMillis, opt_overrideTimeoutMillis) {
    return opt_overrideTimeoutMillis && opt_overrideTimeoutMillis > 0 ?
        opt_overrideTimeoutMillis :
        defaultTimeoutMillis;
  }

  /** @override */
  onBeforeInvokeHandler() {
    mr.EventAnalytics.recordEvent(mr.EventAnalytics.Event.MEDIA_ROUTER);
  }

  /**
   * Helper method to return a string origin from a string or a mojo.Origin
   * object.

   * @param {!mojo.Origin|string} origin
   * @return {string}
   * @private
   */
  mojoOriginToString_(origin) {
    if (typeof origin == 'string') {
      return origin;
    }
    return mr.MojoUtils.mojoOriginToString(/** @type{!mojo.Origin} */ (origin));
  }

  /**
   * @override
   */
  createRoute(
      sourceUrn, sinkId, presentationId, origin = undefined, tabId = undefined,
      timeoutMillis = undefined, offTheRecord = false) {
    const provider = this.getProviderFor_(sourceUrn, sinkId);
    if (!provider) {
      return Promise.reject(new mr.RouteRequestError(
          mr.RouteRequestResultCode.NO_SUPPORTED_PROVIDER,
          'No provider supports createRoute with source: ' + sourceUrn +
              ' and sink: ' + sinkId));
    }
    let originString = undefined;
    if (origin !== undefined) {
      originString = this.mojoOriginToString_(origin);
    }
    timeoutMillis = mr.ProviderManager.getTimeoutToUse_(
        mr.ProviderManager.CREATE_ROUTE_TIMEOUT_MS, timeoutMillis);
    const routePromise = provider.createRoute(
        sourceUrn, sinkId, presentationId, offTheRecord, timeoutMillis,
        originString, tabId);
    return this.addTimeout_(routePromise, timeoutMillis)
        .then(
            route => route,
            /** Error */ err => {
              this.logger_.error('Error creating route.', err);
              throw err;
            });
  }

  /**
   * @override
   */
  connectRouteByRouteId(
      sourceUrn, routeId, presentationId, origin, tabId,
      timeoutMillis = undefined) {
    const provider = this.routeIdToProvider_.get(routeId);
    if (!provider) {
      return Promise.reject(new mr.RouteRequestError(
          mr.RouteRequestResultCode.NO_SUPPORTED_PROVIDER,
          'No provider supports join ' + routeId));
    }

    // connectRouteByRouteId may take a while. Prevent extension suspension.
    const routePromise = provider.connectRouteByRouteId(
        sourceUrn, routeId, presentationId, this.mojoOriginToString_(origin),
        tabId);
    timeoutMillis = mr.ProviderManager.getTimeoutToUse_(
        mr.ProviderManager.JOIN_ROUTE_TIMEOUT_MS_, timeoutMillis);
    return this.addTimeout_(routePromise, timeoutMillis);
  }

  /**
   * @override
   */
  joinRoute(
      sourceUrn, presentationId, origin, tabId, timeoutMillis = undefined,
      offTheRecord = false) {
    const provider = this.getProviderForJoin_(sourceUrn, presentationId);
    if (!provider) {
      return Promise.reject(new mr.RouteRequestError(
          mr.RouteRequestResultCode.NO_SUPPORTED_PROVIDER,
          'No provider supports join ' + presentationId));
    }

    // joinRoute may take a while. Prevent extension suspension.
    timeoutMillis = mr.ProviderManager.getTimeoutToUse_(
        mr.ProviderManager.JOIN_ROUTE_TIMEOUT_MS_, timeoutMillis);
    const routePromise = provider.joinRoute(
        sourceUrn, presentationId, offTheRecord, timeoutMillis,
        this.mojoOriginToString_(origin), tabId);
    return this.addTimeout_(routePromise, timeoutMillis);
  }

  /**
   * @override
   */
  terminateRoute(routeId) {
    const provider = this.routeIdToProvider_.get(routeId);
    if (!provider) {
      return Promise.reject(new mr.RouteRequestError(
          mr.RouteRequestResultCode.ROUTE_NOT_FOUND,
          'Route not found for routeId ' + routeId));
    }
    return this.maybeStopMirrorSession_(routeId).then(
        () => provider.terminateRoute(routeId));
  }

  /**
   * @override
   */
  startObservingMediaSinks(sourceUrn) {
    if (!this.sinkQueries_.has(sourceUrn)) {
      this.sinkQueries_.add(sourceUrn);
      this.providers_.forEach(p => {
        p.startObservingMediaSinks(sourceUrn);
      });
    }
    this.querySinks_(sourceUrn);
  }

  /**
   * @override
   */
  stopObservingMediaSinks(sourceUrn) {
    if (!this.sinkQueries_.delete(sourceUrn)) {
      this.logger_.info('No existing query ' + sourceUrn);
    } else {
      this.doStopObservingMediaSinks_(sourceUrn);
    }
  }

  /**
   * Helper method to stop a sink query on all providers.
   * @param {string} sourceUrn The URN of the media.
   * @private
   */
  doStopObservingMediaSinks_(sourceUrn) {
    this.providers_.forEach(p => {
      p.stopObservingMediaSinks(sourceUrn);
    });
  }

  /**
   * Queries for sinks capable of displaying |sourceUrn|.
   * @param {string} sourceUrn The URN of the media.
   * @private
   */
  querySinks_(sourceUrn) {

    if (sourceUrn == 'urn:x-org.chromium.media:source:tab:-1') {
      this.logger_.warning('No sinks for sourceUrn: ' + sourceUrn);
    } else {
      /** @type {!Map<string, !mr.Sink>} */
      const sinks = new Map();
      let origins = [];
      // Get available sinks from current providers.
      this.providers_.forEach(p => {
        const sinkList = p.getAvailableSinks(sourceUrn);
        if (sinkList.sinks.length > 0) {
          origins = sinkList.origins;
        }
        sinkList.sinks.forEach(sink => {
          // There shouldn't be duplicate sinks. Log a message if we encounter
          // it.
          if (sinks.has(sink.id)) {
            this.logger_.warning(
                'Detected duplicate sink ' + sink.id +
                ' from provider: ' + p.getName());
          } else {
            sinks.set(sink.id, sink);
          }
        });
      });

      this.sendSinksToMr_(sourceUrn, Array.from(sinks.values()), origins || []);
    }
  }

  /**
   * Sends sinks that support |sourceUrn| to media router.
   * @param {string} sourceUrn
   * @param {!Array<!mr.Sink>} sinkList Sinks that support |sourceUrn|
   * @param {!Array<string>} origins Origins that can access the sink list.
   * @private
   */
  sendSinksToMr_(sourceUrn, sinkList, origins) {
    this.logger_.info(
        'Sending ' + sinkList.length + ' sinks to MR for ' + sourceUrn);
    this.mediaRouterService_.onSinksReceived(
        sourceUrn, sinkList, origins.map(mr.MojoUtils.stringToMojoOrigin));
  }

  /**
   * @override
   */
  sendRouteMessage(routeId, message, opt_extraInfo) {
    const provider = this.routeIdToProvider_.get(routeId);
    if (!provider) {
      return Promise.reject(Error(`Invalid route ID ${routeId}`));
    }
    return this.addIgnoredTimeout_(
        provider.sendRouteMessage(routeId, message, opt_extraInfo),
        mr.ProviderManager.SEND_MESSAGE_TIMEOUT_MS_);
  }

  /**
   * @override
   */
  sendRouteBinaryMessage(routeId, data) {
    const provider = this.routeIdToProvider_.get(routeId);
    if (!provider) {
      return Promise.reject(Error(`Invalid route ID ${routeId}`));
    }
    return this.addIgnoredTimeout_(
        provider.sendRouteBinaryMessage(routeId, data),
        mr.ProviderManager.SEND_MESSAGE_TIMEOUT_MS_);
  }

  /**
   * @override
   */
  startListeningForRouteMessages(routeId) {
    this.mrRouteMessageSender_.listenForRouteMessages(routeId);
  }

  /**
   * @override
   */
  stopListeningForRouteMessages(routeId) {
    this.mrRouteMessageSender_.stopListeningForRouteMessages(routeId);
  }

  /**
   * @override
   */
  detachRoute(routeId) {
    const provider = this.routeIdToProvider_.get(routeId);
    if (!provider) {
      this.logger_.info('Route ' + routeId + ' does not exist.');
      return;
    }
    provider.detachRoute(routeId);
  }

  /**
   * @override
   */
  enableMdnsDiscovery() {
    this.mdnsEnabled_ = true;
    this.mdnsEnabledCallbacks_.forEach(callback => {
      callback();
    });
    this.mdnsEnabledCallbacks_.length = 0;
  }

  /**
   * @param {string} sourceUrn The URN of the media being displayed.
   * @param {string} sinkId
   * @return {?mr.Provider} The provider that can handle |sourceUrn| on |sinkId|.
   *  Null if none exists.
   * @private
   */
  getProviderFor_(sourceUrn, sinkId) {
    return this.providers_.find(
               provider => provider.canRoute(sourceUrn, sinkId)) ||
        null;
  }

  /**
   * @param {string} sourceUrn The URN of the media being displayed.
   * @param {string} presentationId The presentation ID to join.
   * @return {?mr.Provider} The provider that can join a route identified by
   *  |sourceUrn| and |presentationId|. Null if none exists.
   * @private
   */
  getProviderForJoin_(sourceUrn, presentationId) {
    return this.providers_.find(
               provider => provider.canJoin(sourceUrn, presentationId)) ||
        null;
  }

  /**
   * @private
   */
  executeSinkQueries_() {
    this.sinkQueries_.forEach(sourceUrn => {
      this.querySinks_(sourceUrn);
    });
  }

  /**
   * @private
   */
  maybeUpdateKeepAlive_() {
    this.mediaRouterService_.setKeepAlive(
        this.pendingRequestRoutes_ > 0 || this.keepAliveComponents_.length > 0);
  }

  /**
   * @override
   */
  startObservingMediaRoutes(sourceUrn) {
    if (!this.routeQueries_.has(sourceUrn)) {
      this.routeQueries_.add(sourceUrn);
      this.providers_.forEach(p => {
        p.startObservingMediaRoutes(sourceUrn);
      });
    }
    this.routeUpdateEventThrottle_.fire();
  }

  /**
   * @override
   */
  stopObservingMediaRoutes(sourceUrn) {
    if (!this.routeQueries_.delete(sourceUrn)) {
      this.logger_.info('No existing route query ' + sourceUrn);
    } else {
      this.providers_.forEach(provider => {
        provider.stopObservingMediaRoutes(sourceUrn);
      });
    }
  }

  /**
   * Send routes query result to MR immediately.
   * @private
   */
  sendRoutesQueryResultToMr_() {
    if (this.routeQueries_.size == 0) return;

    let routeList = [];
    this.providers_.forEach(p => {
      routeList = routeList.concat(p.getRoutes());
    });

    this.routeQueries_.forEach(sourceUrn => {
      const nonLocalJoinableRouteIds = [];
      this.providers_.forEach(p => {
        const routes = p.getRoutes();
        routes.forEach(route => {
          if (!route.createdLocally && p.canJoin(sourceUrn, undefined, route)) {
            nonLocalJoinableRouteIds.push(route.id);
          }
        });
      });
      this.mediaRouterService_.onRoutesUpdated(
          routeList, sourceUrn, nonLocalJoinableRouteIds);
    });
  }

  /**
   * @override
   */
  getStorageKey() {
    return 'ProviderManager';
  }

  /**
   * @override
   */
  getData() {
    return [new mr.ProviderManager.PersistentData_(
        this.providers_.map(p => p.getName()), Array.from(this.sinkQueries_),
        Array.from(this.routeQueries_),
        Array.from(
            this.routeIdToProvider_,
            ([routeId, provider]) => [routeId, provider.getName()]),
        Array.from(this.sinkAvailabilityMap_), this.mdnsEnabled_,
        this.lastUsedMirrorService_)];
  }

  /**
   * @override
   */
  loadSavedData() {
    const savedData = /** @type {mr.ProviderManager.PersistentData_} */
        (mr.PersistentDataManager.getTemporaryData(this));
    if (savedData) {
      this.sinkQueries_ = new Set(savedData.sinkQueries);
      this.routeQueries_ = new Set(savedData.routeQueries);

      for (let [routeId, providerName] of savedData.routeIdToProviderName) {
        const provider = this.getProviderByName(providerName);
        mr.Assertions.assert(provider, 'Provider not found: ' + providerName);
        this.routeIdToProvider_.set(routeId, provider);
      }
      this.sinkAvailabilityMap_ = new Map(savedData.sinkAvailabilityMap);
      this.lastUsedMirrorService_ = savedData.lastUsedMirrorService || null;
      if (savedData.mdnsEnabled) {
        this.enableMdnsDiscovery();
      }
    }
  }

  /**
   * @override
   */
  getProviderFromRouteId(routeId) {
    return this.routeIdToProvider_.get(routeId);
  }

  /**
   * @override
   */
  onRouteAdded(provider, route) {
    this.routeIdToProvider_.set(route.id, provider);
    this.onRouteUpdated(provider, route);
  }

  /**
   * @override
   */
  onRouteRemoved(provider, route) {
    // Stopping mirroring does not affect message delivery.
    this.maybeStopMirrorSession_(route.id);

    this.routeIdToProvider_.delete(route.id);
    this.mrRouteMessageSender_.onRouteRemoved(route.id);
    this.onRouteUpdated(provider, route);
  }

  /**
   * @override
   */
  onRouteUpdated(provider, route) {
    if (this.routeQueries_.size > 0) this.routeUpdateEventThrottle_.fire();
  }

  /**
   * @override
   */
  handleMirrorActivityUpdate(route, mirrorActivity) {
    const provider = this.routeIdToProvider_.get(route.id);
    if (provider) {
      provider.onMirrorActivityUpdated(route.id);
      // Bypass the throttle, since an update to the route description may have
      // happened before the throttle interval has elapsed.
      if (this.routeQueries_.size > 0) this.sendRoutesQueryResultToMr_();
    }
  }

  /**
   * @override
   */
  onRouteMessage(provider, routeId, message) {
    if (!this.routeIdToProvider_.has(routeId)) {
      this.logger_.warning('Got route message for closed route ' + routeId);
      return;
    }
    // If message is not a string or an Uint8Array, encode it into a JSON string
    // for mr.cast.InternalMessage.
    if ((typeof message !== 'string') && !(message instanceof Uint8Array)) {
      message = JSON.stringify(message);
    }
    this.mrRouteMessageSender_.send(routeId, message);
  }

  /**
   * @override
   */
  onPresentationConnectionStateChanged(routeId, state) {
    if (state == mr.PresentationConnectionState.TERMINATED) {
      this.mrRouteMessageSender_.sendImmediately();
    }
    this.mediaRouterService_.onPresentationConnectionStateChanged(
        routeId, /** @type {string} */ (state));
  }

  /**
   * @override
   */
  onPresentationConnectionClosed(routeId, reason, message) {
    this.mrRouteMessageSender_.sendImmediately();
    this.mediaRouterService_.onPresentationConnectionClosed(
        routeId, /** @type {string} */ (reason), message);
  }

  /**
   * @override
   */
  onMirrorSessionEnded(routeId) {
    // When mirror service invokes this method, it already cleaned its session.
    // So provider manager only needs to close the route via provider and does
    // not
    // need to invoke mirrorService.stopCurrentMirroring.
    this.routeIdToMirrorServiceName_.delete(routeId);
    const provider = this.routeIdToProvider_.get(routeId);
    if (provider) {
      provider.terminateRoute(routeId);
    }
  }

  /**
   * @override
   */
  onInternalMessage(provider, routeId, message) {
    this.routeMessageEventTarget_.dispatchEvent(
        new mr.InternalMessageEvent(routeId, message));
  }

  /**
   * @override
   */
  onSinksUpdated() {
    this.sinkUpdateEventThrottle_.fire();
  }

  /**
   * @override
   */
  onSinkAvailabilityUpdated(provider, availability) {
    const oldValue = this.sinkAvailabilityMap_.get(provider.getName());
    mr.Assertions.assert(oldValue !== undefined, 'oldValue != undefined');
    if (oldValue == availability) {
      return;
    }

    const currentAvailability = this.computeSinkAvailability_();
    this.sinkAvailabilityMap_.set(provider.getName(), availability);
    const newAvailability = this.computeSinkAvailability_();
    if (currentAvailability == newAvailability) {
      return;
    }

    // When the overall SinkAvailability becomes UNAVAILABLE, all sink queries
    // will be removed. Before that happens, we need to update the sink query
    // results with empty results.  Importantly, however, this also allows
    // pseudo sinks to be sent as well, which clearing the sinks on the browser
    // side would not maintain.
    if (newAvailability == mr.SinkAvailability.UNAVAILABLE) {
      this.executeSinkQueries_();
    }
    this.mediaRouterService_.onSinkAvailabilityUpdated(newAvailability);
    if (newAvailability == mr.SinkAvailability.UNAVAILABLE) {
      this.sinkQueries_.forEach(sourceUrn => {
        this.doStopObservingMediaSinks_(sourceUrn);
      });
      this.sinkQueries_.clear();
    }
  }

  /**
   * @return {!mr.SinkAvailability} Overall sink availability based on providers'
   *     availability.
   * @private
   */
  computeSinkAvailability_() {
    return Array.from(this.sinkAvailabilityMap_.values())
        .reduce(
            (prev, cur) => Math.max(prev, cur),
            mr.SinkAvailability.UNAVAILABLE);
  }

  /**
   * @override
   */
  getRouteMessageEventTarget() {
    return this.routeMessageEventTarget_;
  }

  /**
   * @override
   */
  sendIssue(issue) {
    this.mediaRouterService_.onIssue(issue);
  }

  /**
   * @param {string} routeId
   * @return {!Promise<boolean>} Fulfilled with true if the route is for
   *    a mirror session and the mirroring was stopped, and with false
   *    otherwise.
   * @private
   */
  maybeStopMirrorSession_(routeId) {
    if (this.routeIdToMirrorServiceName_.has(routeId)) {
      // Stop mirroring first
      return this
          .getMirrorService(this.routeIdToMirrorServiceName_.get(routeId))
          .then(mirrorService => {
            this.routeIdToMirrorServiceName_.delete(routeId);
            return mirrorService.stopCurrentMirroring();
          });
    }
    return Promise.resolve(false);
  }

  /**
   * @override
   */
  startMirroring(
      provider, route, opt_presentationId, opt_streamStartedCallback) {
    // The provider can handle the mirroring URN, thus the mirror
    // service name is non-null.
    const mirrorServiceName =
        /** @type {mr.mirror.ServiceName} */ (mr.Assertions.assertString(
            provider.getMirrorServiceName(route.sinkId)));
    this.logger_.info(`Starting mirroring using service: ${mirrorServiceName}`);
    this.routeIdToMirrorServiceName_.set(route.id, mirrorServiceName);
    let innerPromise = null;
    let cancellationReason = null;
    return mr.CancellablePromise.withUncancellableStep(
        this.getMirrorService(mirrorServiceName), mirrorService => {
          this.lastUsedMirrorService_ = mirrorServiceName;
          return mirrorService
              .startMirroring(

                  route, mr.Assertions.assertString(route.mediaSource),
                  provider.getMirrorSettings(route.sinkId), opt_presentationId,
                  opt_streamStartedCallback)
              .catch(err => {
                if (err instanceof mr.mirror.Error &&
                    err.reason ==
                        mr.MirrorAnalytics.CapturingFailure
                            .CAPTURE_DESKTOP_FAIL_ERROR_USER_CANCEL) {
                  throw new mr.RouteRequestError(
                      mr.RouteRequestResultCode.CANCELLED);
                }
                throw err;
              });
        });
  }

  /**
   * @override
   */
  updateMirroring(
      provider, route, sourceUrn, opt_presentationId, opt_tabId,
      opt_streamStartedCallback) {
    const mirrorServiceName = this.routeIdToMirrorServiceName_.get(route.id);
    if (!mirrorServiceName) {
      return mr.CancellablePromise.reject(
          Error('Route ' + route.id + ' is not mirroring'));
    }
    return mr.CancellablePromise.withUncancellableStep(
        this.getMirrorService(mirrorServiceName), mirrorService => {
          this.lastUsedMirrorService_ = mirrorServiceName;
          return mirrorService.updateMirroring(
              route, sourceUrn, provider.getMirrorSettings(route.sinkId),
              opt_presentationId, opt_tabId, opt_streamStartedCallback);
        });
  }

  /**
   * @override
   */
  registerMdnsDiscoveryEnabledCallback(callback) {
    if (this.mdnsEnabled_) {
      callback();
      return;
    }
    if (this.mdnsEnabledCallbacks_.indexOf(callback) != -1) {
      return;
    }
    this.mdnsEnabledCallbacks_.push(callback);
  }

  /**
   * @override
   */
  isMdnsDiscoveryEnabled() {
    return this.mdnsEnabled_;
  }

  /**
   * @override
   */
  requestKeepAlive(componentId, keepAlive) {
    const index = this.keepAliveComponents_.indexOf(componentId);
    const componentKeepAlive = (index >= 0);
    if (keepAlive && !componentKeepAlive) {
      this.keepAliveComponents_.push(componentId);
    } else if (!keepAlive && componentKeepAlive) {
      this.keepAliveComponents_.splice(index, 1);
    }
    this.maybeUpdateKeepAlive_();
  }

  /**
   * @param {!string} name
   * @return {?mr.Provider}
   */
  getProviderByName(name) {
    return this.providers_.find(p => p.getName() == name) || null;
  }

  /**
   * Returns the name of the provider that created the pseudo sink ID |sinkId|
   * or null if it is not a valid pseudo sink ID.
   * @param {string} sinkId
   * @return {?string}
   * @private
   */
  getProviderNameFromPseudoSinkId_(sinkId) {
    if (!sinkId.startsWith(mr.ProviderManager.PSEUDO_SINK_NAME_PREFIX_)) {
      return null;
    }
    return sinkId.substring(mr.ProviderManager.PSEUDO_SINK_NAME_PREFIX_.length);
  }

  /**
   * Get the rejection promise when sink is not found.
   * @param {string} sinkId Sink ID of the pseudo sink that generated the
   *   request.
   * @param {string} sourceUrn Source to be used with the sink.
   * @param {!mr.SinkSearchCriteria} searchCriteria Sink search criteria for the
   *   MRP's which includes the user's current domain.
   * @return {!Promise}
   * @private
   */
  rejectWithSinkNotFoundError_(sinkId, sourceUrn, searchCriteria) {
    this.mediaRouterService_.onSearchSinkIdReceived(sinkId, '');
    return Promise.reject(new mr.RouteRequestError(
        mr.RouteRequestResultCode.UNKNOWN_ERROR,
        'No sink found for search input: ' + searchCriteria.input +
            ' and source: ' + sourceUrn));
  }

  /**
   * @override
   */
  searchSinks(sinkId, sourceUrn, searchCriteria) {
    const providerName = this.getProviderNameFromPseudoSinkId_(sinkId);
    const searchOwner =
        providerName ? this.getProviderByName(providerName) : null;
    if (!searchOwner) {
      return Promise.reject(new Error('No provider supports ' + sinkId));
    }
    return searchOwner.searchSinks(sourceUrn, searchCriteria)
        .then((sink) => sink.id);
  }

  /**
   * @override
   */
  provideSinks(providerName, sinks) {
    const provider = this.getProviderByName(providerName);
    if (!provider) {
      this.logger_.error(
          `provideSinks: Provider not found for providerName ${providerName}`);
      return;
    }
    provider.provideSinks(sinks);
  }

  /**
   * @override
   */
  updateMediaSinks(sourceUrn) {
    // Don't add to sinkQueries as the providers may already be unavailable, but
    // if the sink queries already contain the sourceUrn just return as
    // discovery
    // is already ongoing.
    if (this.sinkQueries_.has(sourceUrn)) {
      return;
    }
    this.querySinks_(sourceUrn);
  }

  /**
   * @override
   */
  createMediaRouteController(routeId, controllerRequest, observer) {
    const cleanUpOnError = () => {
      controllerRequest.close();
      observer.ptr.reset();
    };
    const provider = this.routeIdToProvider_.get(routeId);
    if (!provider) {
      const errorMessage =
          `createMediaRouteController: Provider not found for ${routeId}`;
      this.logger_.error(errorMessage);
      cleanUpOnError();
      return Promise.reject(new Error(errorMessage));
    }
    return provider
        .createMediaRouteController(routeId, controllerRequest, observer)
        .catch(e => {
          this.logger_.error(`createMediaRouteController failed: ${e.message}`);
          cleanUpOnError();
          throw e;
        });
  }

  /**
   * @param {number} tabId
   * @param {!mojo.MirrorServiceRemoterPtr} remoter
   * @param {!mojo.InterfaceRequest} remotingSourceRequest
   */
  onMediaRemoterCreated(tabId, remoter, remotingSourceRequest) {
    this.mediaRouterService_.onMediaRemoterCreated(
        tabId, remoter, remotingSourceRequest);
  }

  /**
   * @return {?mr.mirror.ServiceName} The name of most recently used mirror
   *     service, or null if mirror service has not been used.
   */
  getLastUsedMirrorService() {
    return this.lastUsedMirrorService_;
  }

  /**
   * Gets current status of media sink service from browser.
   * @return {!Promise<!{status: string}>}
   */
  getMediaSinkServiceStatus() {
    if (typeof this.mediaRouterService_.getMediaSinkServiceStatus !=
        'function') {
      return Promise.resolve(/** @type {{status: string}} */ ({status: ''}));
    }
    return this.mediaRouterService_.getMediaSinkServiceStatus();
  }

  /**
   * Exports methods for Mojo handler.
   * @param {!mr.ProviderManager} providerManager
   * @private
   */
  static exportProperties_(providerManager) {
    // Cast to {!Object} so the compiler doesn't complain about accessing fields
    // using subscript notation.
    const obj = /** @type {!Object} */ (providerManager);
    obj['onBeforeInvokeHandler'] = providerManager.onBeforeInvokeHandler;
    obj['createRoute'] = providerManager.createRoute;
    obj['joinRoute'] = providerManager.joinRoute;
    obj['connectRouteByRouteId'] = providerManager.connectRouteByRouteId;
    obj['terminateRoute'] = providerManager.terminateRoute;
    obj['startObservingMediaSinks'] = providerManager.startObservingMediaSinks;
    obj['stopObservingMediaSinks'] = providerManager.stopObservingMediaSinks;
    obj['sendRouteMessage'] = providerManager.sendRouteMessage;
    obj['sendRouteBinaryMessage'] = providerManager.sendRouteBinaryMessage;
    obj['startListeningForRouteMessages'] =
        providerManager.startListeningForRouteMessages;
    obj['stopListeningForRouteMessages'] =
        providerManager.stopListeningForRouteMessages;
    obj['startObservingMediaRoutes'] =
        providerManager.startObservingMediaRoutes;
    obj['stopObservingMediaRoutes'] = providerManager.stopObservingMediaRoutes;
    obj['detachRoute'] = providerManager.detachRoute;
    obj['enableMdnsDiscovery'] = providerManager.enableMdnsDiscovery;
    obj['searchSinks'] = providerManager.searchSinks;
    obj['provideSinks'] = providerManager.provideSinks;
    obj['updateMediaSinks'] = providerManager.updateMediaSinks;
    obj['createMediaRouteController'] =
        providerManager.createMediaRouteController;
  }
};

/**
 * Provider may fire sink/route list change event frequently.  To avoid run all
 * sink queries too frequently, only one set of queries at the end of each
 * interval if there is an update event in the interval.
 * @private @const {number}
 */
mr.ProviderManager.ALL_QUERIES_INTERVAL_MS_ = 500;

// The timeout values below are to ensure provider manager resolve or reject
// a pending MR request after a reasonable delay. This is a safe guard in case
// provider has some unforeseen error.

/** @const {number} */
mr.ProviderManager.CREATE_ROUTE_TIMEOUT_MS = 60 * 1000;

/** @private @const {number} */
mr.ProviderManager.JOIN_ROUTE_TIMEOUT_MS_ = 30 * 1000;

/** @private @const {number} */
mr.ProviderManager.SEND_MESSAGE_TIMEOUT_MS_ = 30 * 1000;

/** @private @const {string} */
mr.ProviderManager.PSEUDO_SINK_NAME_PREFIX_ = 'pseudo:';


/**
 * The Data to be saved in storage when event page suspends.
 * @private
 */
mr.ProviderManager.PersistentData_ = class {
  /**
   * @param {!Array<string>} providerNames
   * @param {!Array<string>} sinkQueries
   * @param {!Array<string>} routeQueries
   * @param {!Array<!Array>} routeIdToProviderName
   * @param {!Array<!Array>} sinkAvailabilityMap
   * @param {boolean} mdnsEnabled
   * @param {?mr.mirror.ServiceName} lastUsedMirrorService
   */
  constructor(
      providerNames, sinkQueries, routeQueries, routeIdToProviderName,
      sinkAvailabilityMap, mdnsEnabled, lastUsedMirrorService) {
    /**
     * @type {!Array<string>}
     */
    this.providerNames = providerNames;

    /**
     * @type {!Array<string>}
     */
    this.sinkQueries = sinkQueries;

    /**
     * @type {!Array<string>}
     */
    this.routeQueries = routeQueries;

    /**
     * Map encoded as an array.
     * @type {!Array<!Array>}
     */
    this.routeIdToProviderName = routeIdToProviderName;

    /**
     * Map encoded as an array.
     * @type {!Array<!Array>}
     */
    this.sinkAvailabilityMap = sinkAvailabilityMap;

    /**
     * @type {boolean}
     */
    this.mdnsEnabled = mdnsEnabled;

    /**
     * @type {?mr.mirror.ServiceName}
     */
    this.lastUsedMirrorService = lastUsedMirrorService;
  }
};
