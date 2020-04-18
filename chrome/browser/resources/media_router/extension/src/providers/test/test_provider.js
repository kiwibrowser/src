// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview API for test media route provider.
 *
 * This media route provider is only used for integration test in chromium
 * waterfall.
 */

goog.provide('mr.TestProvider');

goog.require('mr.Assertions');
goog.require('mr.CancellablePromise');
goog.require('mr.Logger');
goog.require('mr.MediaSourceUtils');
goog.require('mr.PresentationConnectionCloseReason');
goog.require('mr.PresentationConnectionState');
goog.require('mr.Provider');
goog.require('mr.ProviderName');
goog.require('mr.Route');
goog.require('mr.RouteRequestError');
goog.require('mr.Sink');
goog.require('mr.SinkAvailability');
goog.require('mr.SinkList');


/**
 * @implements {mr.Provider}
 */
mr.TestProvider = class {
  /**
   * @param {!mr.ProviderManagerCallbacks} providerManagerCallbacks
   * @final
   */
  constructor(providerManagerCallbacks) {
    /** @private {!mr.ProviderManagerCallbacks} */
    this.providerManagerCallbacks_ = providerManagerCallbacks;

    /** @private {!Array<!mr.Sink>} */
    this.sinks_ =
        [new mr.Sink('id1', 'test-sink-1'), new mr.Sink('id2', 'test-sink-2')];

    /**
     * Maps from mr.Route ID to mr.Route. Includes closed routes but not
     * terminated routes.
     * @private {!Map<string, !mr.Route>} */
    this.routes_ = new Map();

    /**
     * Maps from presentation ID to mr.Route. Includes closed routes but not
     * terminated routes.
     * @private {!Map<string, !mr.Route>} */
    this.presentationIdToRoute_ = new Map();

    /**
     * Holds the MediaStream returned by chrome.tabCapture.captureOffscreenTab.
     * This prevents the offscreen tab from being destroyed while the test is
     * running.
     * @private {?MediaStream}
     */
    this.stream_ = null;
  }

  /**
   * @override
   */
  getName() {
    return mr.ProviderName.TEST;
  }

  /**
   * @override
   */
  initialize() {
    setTimeout(this.loadSinksAndComputeAvailability_.bind(this));
  }

  /**
   * @private
   */
  loadSinksAndComputeAvailability_() {
    const testData = /** @type {Array} */ (this.getTestData_('initialSinks'));
    if (testData) {
      this.sinks_ = [];
      testData.forEach(sink => {
        this.sinks_.push(new mr.Sink(sink['id'], sink['friendlyName']));
      });
    }

    this.providerManagerCallbacks_.onSinkAvailabilityUpdated(
        this,
        this.sinks_.length == 0 ? mr.SinkAvailability.UNAVAILABLE :
                                  mr.SinkAvailability.PER_SOURCE);
  }

  /**
   * @override
   */
  getRoutes() {
    return Array.from(this.routes_.values());
  }

  /**
   * @override
   */
  getAvailableSinks(sourceUrn) {
    const testData =
        /** @type {Object} */ (this.getTestData_('getAvailableSinks'));
    if (!testData) {
      if (!this.isValidSource_(sourceUrn)) {
        return mr.SinkList.EMPTY;
      }
      return new mr.SinkList(this.sinks_);
    }

    const testSinks = /** @type {Array} */ (testData[sourceUrn]);
    if (!testSinks) {
      return mr.SinkList.EMPTY;
    }

    const sinks = [];
    testSinks.forEach(sink => {
      sinks.push(new mr.Sink(sink['id'], sink['friendlyName']));
    });
    return new mr.SinkList(sinks);
  }

  /**
   * @override
   */
  startObservingMediaSinks(sourceUrn) {}

  /**
   * @override
   */
  stopObservingMediaSinks(sourceUrn) {}

  /**
   * @override
   */
  startObservingMediaRoutes() {}

  /**
   * @override
   */
  stopObservingMediaRoutes() {}

  /**
   * @override
   */
  getSinkById(id) {
    return this.sinks_.find(s => {
      return s.id == id;
    }) ||
        null;
  }

  /**
   * @override
   */
  createRoute(
      sourceUrn, sinkId, presentationId, offTheRecord, timeoutMillis,
      opt_origin, opt_tabId) {
    const rejectingPromise = this.getRejectingPromise_('createRoute');
    if (rejectingPromise) {
      return mr.CancellablePromise.forPromise(
          /** @type {!Promise<!mr.Route>} */ (rejectingPromise));
    }

    const testData = /** @type {Object} */ (this.getTestData_('createRoute'));
    if (testData && 'delayMillis' in testData) {
      return new mr.CancellablePromise((resolve, reject) => {
        setTimeout(() => {
          resolve(this.doCreateRoute_(
              sourceUrn, sinkId, presentationId, offTheRecord, opt_origin,
              opt_tabId));
        }, testData['delayMillis']);
      });
    } else {
      return mr.CancellablePromise.resolve(this.doCreateRoute_(
          sourceUrn, sinkId, presentationId, offTheRecord, opt_origin,
          opt_tabId));
    }
  }

  /**
   * @param {string} sourceUrn
   * @param {string} sinkId The ID of the target sink.
   * @param {!string} presentationId A presentation ID to use.
   * @param {boolean} offTheRecord True if the route is off-the-record.
   * @param {!string=} opt_origin
   * @param {!number=} opt_tabId
   * @return {!mr.Route} The created route.
   * @private
   */
  doCreateRoute_(
      sourceUrn, sinkId, presentationId, offTheRecord, opt_origin, opt_tabId) {
    const route = mr.Route.createRoute(
        presentationId, this.getName(), sinkId, sourceUrn, true, 'Test Route',
        null);
    route.offTheRecord = offTheRecord;
    this.routes_.set(route.id, route);
    this.presentationIdToRoute_.set(presentationId, route);
    this.providerManagerCallbacks_.onRouteAdded(this, route);
    this.providerManagerCallbacks_.requestKeepAlive(
        mr.TestProvider.COMPONENT_ID, true);

    if (mr.MediaSourceUtils.isPresentationSource(sourceUrn)) {
      route.isOffscreenPresentation = true;
      this.captureOffscreenTab_(sourceUrn, presentationId);
    }
    return route;
  }

  /**
   * Start an offscreen tab.
   * @param {string} offscreenTabUrl
   * @param {string} presentationId
   * @private
   */
  captureOffscreenTab_(offscreenTabUrl, presentationId) {
    const captureOptions = /** @type {!MediaConstraints} */ ({
      video: true,
      audio: false,
      videoConstraints: {
        mandatory: {
          minWidth: 180,
          minHeight: 180,
          maxWidth: 1920,
          maxHeight: 1080,
          maxFrameRate: 30,
        }
      },
      presentationId: presentationId
    });

    chrome.tabCapture.captureOffscreenTab(
        offscreenTabUrl, captureOptions, stream => {
          // Store a reference to the returned stream to keep it alive.
          this.stream_ = stream;
        });
  }

  /**
   * @override
   */
  terminateRoute(routeId) {
    const route = this.routes_.get(routeId);
    if (!route) {
      return Promise.reject(new mr.RouteRequestError(
          mr.RouteRequestResultCode.ROUTE_NOT_FOUND,
          'Route in test provider not found for routeId ' + routeId));
    }
    this.removeRoute_(route);
    this.removeRouteFromMapping_(routeId);
    this.providerManagerCallbacks_.onPresentationConnectionStateChanged(
        routeId, mr.PresentationConnectionState.TERMINATED);
    return Promise.resolve();
  }

  /**
   * @override
   */
  sendRouteMessage(routeId, message, opt_extraInfo) {
    const testData =
        /** @type {Object} */ (this.getTestData_('closeRouteWithErrorOnSend'));
    if (testData == 'true') {
      // This must be called after this function returns, so that the test does
      // not terminate before settling the promise.
      setTimeout(() => {
        this.providerManagerCallbacks_.onPresentationConnectionClosed(
            routeId, mr.PresentationConnectionCloseReason.ERROR, 'Foo');
        const route = this.routes_.get(routeId);
        if (route) this.providerManagerCallbacks_.onRouteRemoved(this, route);
      }, 0);
      return Promise.reject(Error('Send error. Closing connection.'));
    } else {
      // This must be called after this function returns, so that the test does
      // not terminate before settling the promise.
      setTimeout(() => {
        // Send a message back in response.
        const response = 'Pong: ' + message;
        this.providerManagerCallbacks_.onRouteMessage(this, routeId, response);
      }, 0);
      return Promise.resolve();
    }
  }

  /**
   * @override
   */
  sendRouteBinaryMessage(routeId, message) {
    return Promise.reject(
        Error(`Route ${routeId} does not support sending binary data.`));
  }

  /**
   * @override
   */
  getMirrorSettings(sinkId) {
    throw new Error('Not implemented.');
  }

  /**
   * @override
   */
  getMirrorServiceName(sinkId) {
    return null;
  }

  /**
   * @override
   */
  onMirrorActivityUpdated(routeId) {}

  /**
   * @override
   */
  canRoute(sourceUrn, sinkId) {
    if (!this.isValidSource_(sourceUrn)) {
      return false;
    }
    const testData = /** @type {?string} */ (this.getTestData_('canRoute'));
    if (testData) {
      return testData == 'true';
    }
    return !!this.getSinkById(sinkId);
  }

  /**
   * @override
   */
  canJoin(sourceUrn, presentationId, route) {
    if (!this.isValidSource_(sourceUrn)) {
      return false;
    }
    const testData = /** @type {?string} */ (this.getTestData_('canJoin'));
    if (testData) {
      return testData == 'true';
    }

    // If route is not provided, optimistically return true.
    return route ? this.routes_.has(route.id) : true;
  }

  /**
   * @override
   */
  connectRouteByRouteId(sourceUrn, routeId, presentationId, origin, tabId) {
    if (!this.isValidSource_(sourceUrn)) {
      return mr.CancellablePromise.reject(Error('Invalid source'));
    }
    const testData = this.getRejectingPromise_('connectRouteByRouteId');
    if (testData) {
      return mr.CancellablePromise.forPromise(
          /** @type {!Promise<!mr.Route>} */ (testData));
    }
    const existingRoute = this.routes_.get(routeId);
    if (!existingRoute) {
      return mr.CancellablePromise.reject(Error('Presentation does not exist'));
    }
    return mr.CancellablePromise.resolve(existingRoute);
  }

  /**
   * @override
   */
  joinRoute(
      sourceUrn, presentationId, offTheRecord, timeoutMillis, origin, tabId) {
    if (!this.isValidSource_(sourceUrn)) {
      return mr.CancellablePromise.reject(Error('Invalid source'));
    }
    const testData = this.getRejectingPromise_('joinRoute');
    if (testData) {
      return mr.CancellablePromise.forPromise(
          /** @type {!Promise<!mr.Route>} */ (testData));
    }
    const existingRoute = this.presentationIdToRoute_.get(presentationId);
    if (!existingRoute) {
      return mr.CancellablePromise.reject(Error('Presentation does not exist'));
    }
    if (existingRoute.offTheRecord != offTheRecord) {
      return mr.CancellablePromise.reject(Error('Off-the-record mismatch'));
    }
    return mr.CancellablePromise.resolve(existingRoute);
  }

  /**
   * @override
   */
  detachRoute(routeId) {
    this.providerManagerCallbacks_.onPresentationConnectionClosed(
        routeId, mr.PresentationConnectionCloseReason.CLOSED, 'Close route');
  }

  /**
   * Remove all entries from |this.presentationIdToRoute_| that has the route.
   * @param {!string} routeId The ID of the route to remove.
   * @private
   */
  removeRouteFromMapping_(routeId) {
    const newPresentationIdToRoute = new Map();
    this.presentationIdToRoute_.forEach((route, presentationId) => {
      if (route.id != routeId) {
        newPresentationIdToRoute.set(presentationId, route);
      }
    });
    this.presentationIdToRoute_.clear();
    this.presentationIdToRoute_ = newPresentationIdToRoute;
  }

  /**
   * Removes |route| from the list of routes, and notifies the provider manager
   * of the removal.
   * @param {!mr.Route} route The route to remove.
   * @private
   */
  removeRoute_(route) {
    this.routes_.delete(route.id);
    this.providerManagerCallbacks_.onRouteRemoved(this, route);
    if (this.routes_.size == 0) {
      this.providerManagerCallbacks_.requestKeepAlive(
          mr.TestProvider.COMPONENT_ID, false);
    }
  }

  /**
   * Returns true if it is a source that can be handled by TestProvider.
   * @param {string} sourceUrn The URN of the media being displayed.
   * @return {boolean}
   * @private
   */
  isValidSource_(sourceUrn) {
    return sourceUrn.startsWith('test:') || sourceUrn.startsWith('urn:') ||
        mr.MediaSourceUtils.isPresentationSource(sourceUrn);
  }

  /**
   * Gets test data from localstorage.
   * @param {!string} key the API name as key for the test data.
   * @return {*} value for the specific key, otherwise null.
   * @private
   */
  getTestData_(key) {
    if ('testdata' in window.localStorage) {
      const testdata = JSON.parse(window.localStorage['testdata']);
      if (key in testdata) {
        this.logger_.info(key + ' : ' + JSON.stringify(testdata[key]));
        return testdata[key];
      }
    }
    return null;
  }

  /**
   * Gets return value with Promise type which is immediately rejected.
   *
   * Here is an example JSON string to set result with Promise type for
   * createRoute API:
   * {"testdata":
   *   {"createRoute": {"passed": "false", "errorMessage": "Unkown sink"}}
   * }
   *
   * @param {!string} key the API name as key for the test data.
   * @return {?Promise} the Promise that is immediately rejected with the
   *   given reason.
   * @private
   */
  getRejectingPromise_(key) {
    const testData = /** @type {Object} */ (this.getTestData_(key));
    if (testData) {
      if ('passed' in testData && testData['passed'] == 'false' &&
          'errorMessage' in testData) {
        return Promise.reject(Error(testData['errorMessage']));
      }
    }
    return null;
  }

  /**
   * @override
   */
  searchSinks(sourceUrn, searchCriteria) {
    // Not implemented.
    return mr.Assertions.rejectNotImplemented();
  }

  /**
   * @override
   */
  createMediaRouteController(routeId, controllerRequest, observer) {
    this.logger_.info('createMediaRouteController: not implemented');
    return mr.Assertions.rejectNotImplemented();
  }

  /**
   * @override
   */
  provideSinks(sinks) {}
};


/** @const {string} */
mr.TestProvider.COMPONENT_ID = 'mr.TestProvider';


/** @const @private {mr.Logger} */
mr.TestProvider.prototype.logger_ = mr.Logger.getInstance('mr.TestProvider');
