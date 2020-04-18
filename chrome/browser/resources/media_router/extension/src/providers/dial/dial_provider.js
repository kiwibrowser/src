// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.module('mr.DialProvider');
goog.module.declareLegacyNamespace();

const Activity = goog.require('mr.dial.Activity');
const ActivityRecords = goog.require('mr.dial.ActivityRecords');
const AppDiscoveryService = goog.require('mr.dial.AppDiscoveryService');
const Assertions = goog.require('mr.Assertions');
const CancellablePromise = goog.require('mr.CancellablePromise');
const DeviceCountsProvider = goog.require('mr.DeviceCountsProvider');
const DialAnalytics = goog.require('mr.DialAnalytics');
const DialClient = goog.require('mr.dial.Client');
const DialPresentationUrl = goog.require('mr.dial.PresentationUrl');
const DialSink = goog.require('mr.dial.Sink');
const Logger = goog.require('mr.Logger');
const MediaSourceUtils = goog.require('mr.MediaSourceUtils');
const PresentationConnectionState = goog.require('mr.PresentationConnectionState');
const Provider = goog.require('mr.Provider');
const ProviderCallbacks = goog.require('mr.dial.ProviderCallbacks');
const ProviderManagerCallbacks = goog.require('mr.ProviderManagerCallbacks');
const ProviderName = goog.require('mr.ProviderName');
const Route = goog.require('mr.Route');
const RouteRequestError = goog.require('mr.RouteRequestError');
const RouteRequestResultCode = goog.require('mr.RouteRequestResultCode');
const SinkAppStatus = goog.require('mr.dial.SinkAppStatus');
const SinkAvailability = goog.require('mr.SinkAvailability');
const SinkDiscoveryService = goog.require('mr.dial.SinkDiscoveryService');
const SinkList = goog.require('mr.SinkList');
const SinkUtils = goog.require('mr.SinkUtils');

/**
 * DIAL implementation of Media Route Provider.
 * @implements {Provider}
 * @implements {ProviderCallbacks}
 * @implements {DeviceCountsProvider}
 */
const DialProvider = class {
  /**
   * @param {!ProviderManagerCallbacks} providerManagerCallbacks
   * @param {!SinkDiscoveryService=} sinkDiscoveryService
   * @param {!AppDiscoveryService=} appDiscoveryService
   * @final
   */
  constructor(
      providerManagerCallbacks, sinkDiscoveryService = undefined,
      appDiscoveryService = undefined) {
    /** @private @const {!ProviderManagerCallbacks} */
    this.providerManagerCallbacks_ = providerManagerCallbacks;

    /** @private @const {!SinkDiscoveryService} */
    this.sinkDiscoveryService_ =
        sinkDiscoveryService || new SinkDiscoveryService(this);

    /** @private @const {!ActivityRecords} */
    this.activityRecords_ = new ActivityRecords(this);

    /** @private {?AppDiscoveryService} */
    this.appDiscoveryService_ = appDiscoveryService || null;

    /** @private @const {?Logger} */
    this.logger_ = Logger.getInstance('mr.DialProvider');
  }

  /**
   * @override
   */
  getName() {
    return ProviderName.DIAL;
  }

  /**
   * @override
   */
  getDeviceCounts() {
    return this.sinkDiscoveryService_.getDeviceCounts();
  }

  /**
   * @override
   */
  initialize(config) {
    const sinkQueryEnabled =
        (config && config.enable_dial_sink_query == false) ? false : true;
    this.logger_.info('Dial sink query enabled: ' + sinkQueryEnabled + '...');

    this.activityRecords_.init();
    this.sinkDiscoveryService_.init();

    if (sinkQueryEnabled) {
      this.appDiscoveryService_ = this.appDiscoveryService_ ||
          new AppDiscoveryService(this.sinkDiscoveryService_,
                                  this.activityRecords_);
      this.appDiscoveryService_.init();
    } else {
      this.appDiscoveryService_ = null;
    }

    this.maybeStartAppDiscovery_();
  }

  /**
   * @override
   */
  getAvailableSinks(sourceUrn) {
    // Prevent SinkDiscoveryService to return cached available sinks.
    if (!this.appDiscoveryService_) {
      return SinkList.EMPTY;
    }

    this.logger_.fine('GetAvailableSinks for ' + sourceUrn);
    const dialMediaSource = DialPresentationUrl.create(sourceUrn);
    return dialMediaSource ?
        this.sinkDiscoveryService_.getSinksByAppName(dialMediaSource.appName) :
        SinkList.EMPTY;
  }

  /**
   * @override
   */
  startObservingMediaSinks(sourceUrn) {
    if (!this.appDiscoveryService_) {
      return;
    }

    const dialMediaSource = DialPresentationUrl.create(sourceUrn);
    if (dialMediaSource) {
      this.appDiscoveryService_.registerApp(dialMediaSource.appName);
      this.maybeStartAppDiscovery_();
    }
  }

  /**
   * @override
   */
  stopObservingMediaSinks(sourceUrn) {
    if (!this.appDiscoveryService_) {
      return;
    }

    const dialMediaSource = DialPresentationUrl.create(sourceUrn);
    if (dialMediaSource) {
      this.appDiscoveryService_.unregisterApp(dialMediaSource.appName);
      this.maybeStopAppDiscovery_();
    }
  }

  /**
   * @override
   */
  startObservingMediaRoutes(sourceUrn) {
    this.maybeStartAppDiscovery_();
  }

  /**
   * @override
   */
  stopObservingMediaRoutes(sourceUrn) {
    this.maybeStopAppDiscovery_();
  }

  /**
   * @private
   */
  maybeStopAppDiscovery_() {
    if (!this.appDiscoveryService_) {
      return;
    }

    if (this.sinkDiscoveryService_.getSinkCount() == 0 ||
        (this.appDiscoveryService_.getAppCount() == 0 &&
         this.activityRecords_.getActivityCount() == 0)) {
      this.appDiscoveryService_.stop();
    }
  }

  /**
   * @private
   */
  maybeStartAppDiscovery_() {
    if (!this.appDiscoveryService_) {
      return;
    }

    if (this.sinkDiscoveryService_.getSinkCount() > 0 &&
        (this.appDiscoveryService_.getAppCount() > 0 ||
         this.activityRecords_.getActivityCount() > 0)) {
      this.appDiscoveryService_.start();
    }
  }

  /**
   * @override
   */
  getSinkById(id) {
    const dialSink = this.sinkDiscoveryService_.getSinkById(id);
    return dialSink ? dialSink.getMrSink() : null;
  }

  /**
   * @override
   */
  getRoutes() {
    return this.activityRecords_.getRoutes();
  }

  /**
   * @override
   */
  createRoute(
      sourceUrn, sinkId, presentationId, offTheRecord, timeoutMillis,
      opt_origin, opt_tabId) {
    const sink = this.sinkDiscoveryService_.getSinkById(sinkId);
    if (!sink) {
      DialAnalytics.recordCreateRoute(
          DialAnalytics.DialRouteCreation.FAILED_NO_SINK);
      return CancellablePromise.reject(Error('Unkown sink: ' + sinkId));
    }
    SinkUtils.getInstance().recentLaunchedDevice =
        new SinkUtils.DeviceData(sink.getModelName(), sink.getIpAddress());
    const dialMediaSource = DialPresentationUrl.create(sourceUrn);
    if (!dialMediaSource) {
      return CancellablePromise.reject(Error('No app name set.'));
    }
    const appName = dialMediaSource.appName;
    const dialClient = this.newClient_(sink);

    return CancellablePromise.forPromise(
        /** @type {!Promise<!Route>} */
        (dialClient.getAppInfo(appName)
             .then(appInfo => {
               if (appInfo.state == DialClient.DialAppState.RUNNING) {
                 return dialClient.stopApp(appName);
               }
             })
             .then(() => {
               return dialClient.launchApp(
                   appName, dialMediaSource.launchParameter);
             })
             .then(() => {
               return this.addRoute(
                   sinkId, sourceUrn, true, appName, presentationId,
                   offTheRecord);
             })
             .catch(err => {
               DialAnalytics.recordCreateRoute(
                   DialAnalytics.DialRouteCreation.FAILED_LAUNCH_APP);
               throw err;
             })));
  }

  /**
   * @override
   */
  joinRoute(
      sourceUrn, presentationId, offTheRecord, timeoutMillis, origin, tabId) {
    return CancellablePromise.reject(Error('Not supported'));
  }

  /**
   * @override
   */
  connectRouteByRouteId(sourceUrn, routeId, presentationId, origin, tabId) {
    return CancellablePromise.reject(Error('Not supported'));
  }

  /**
   * @override
   */
  detachRoute(routeId) {}

  /**
   * @param {string} sinkId
   * @param {?string} sourceUrn
   * @param {boolean} isLocal
   * @param {string} appName
   * @param {string} presentationId
   * @param {boolean} offTheRecord
   * @return {!Route} The route that was just added.
   */
  addRoute(sinkId, sourceUrn, isLocal, appName, presentationId, offTheRecord) {
    DialAnalytics.recordCreateRoute(
        DialAnalytics.DialRouteCreation.ROUTE_CREATED);
    const route = Route.createRoute(
        presentationId, this.getName(), sinkId, sourceUrn, isLocal, appName,
        null);
    route.offTheRecord = offTheRecord;
    this.activityRecords_.add(new Activity(route, appName));
    return route;
  }

  /**
   * @override
   */
  onSinkAdded(sink) {
    this.providerManagerCallbacks_.onSinkAvailabilityUpdated(
        this, SinkAvailability.PER_SOURCE);
    if (this.appDiscoveryService_) {
      this.maybeStartAppDiscovery_();
      this.appDiscoveryService_.scanSink(sink);
    }
    this.providerManagerCallbacks_.onSinksUpdated();
    SinkUtils.getInstance().recentDiscoveredDevice =
        new SinkUtils.DeviceData(sink.getModelName(), sink.getIpAddress());
  }

  /**
   * @override
   */
  onSinksRemoved(sinks) {
    if (this.sinkDiscoveryService_.getSinkCount() == 0) {
      this.providerManagerCallbacks_.onSinkAvailabilityUpdated(
          this, SinkAvailability.UNAVAILABLE);
    }
    this.maybeStopAppDiscovery_();
    sinks.forEach(sink => {
      this.activityRecords_.removeBySinkId(sink.getId());
    });
    this.providerManagerCallbacks_.onSinksUpdated();
  }

  /**
   * @override
   */
  onSinkUpdated(sink) {
    this.providerManagerCallbacks_.onSinksUpdated();
  }

  /**
   * @override
   */
  onActivityAdded(activity) {
    this.maybeStartAppDiscovery_();
    this.providerManagerCallbacks_.onRouteAdded(this, activity.route);
  }

  /**
   * @override
   */
  onActivityRemoved(activity) {
    const route = activity.route;
    if (route.isLocal) {
      this.providerManagerCallbacks_.onPresentationConnectionStateChanged(
          route.id, PresentationConnectionState.TERMINATED);
    }
    this.maybeStopAppDiscovery_();
    this.providerManagerCallbacks_.onRouteRemoved(this, route);
  }

  /**
   * @override
   */
  onActivityUpdated(activity) {
    this.providerManagerCallbacks_.onRouteUpdated(this, activity.route);
  }

  /**
   * @override
   */
  terminateRoute(routeId) {
    const activity = this.activityRecords_.getByRouteId(routeId);
    if (!activity) {
      return Promise.reject(new RouteRequestError(
          RouteRequestResultCode.ROUTE_NOT_FOUND,
          'Route in DIAL provider not found for routeId ' + routeId));
    }
    this.activityRecords_.removeByRouteId(routeId);
    const sink = this.sinkDiscoveryService_.getSinkById(activity.route.sinkId);
    if (!sink) {
      return Promise.reject(new RouteRequestError(
          RouteRequestResultCode.ROUTE_NOT_FOUND,
          'Sink in DIAL provider not found for sinkId ' +
              activity.route.sinkId));
    }
    return this.newClient_(sink).stopApp(activity.appName);
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
  sendRouteMessage(routeId, message, opt_extraInfo) {
    return Promise.reject(Error('DIAL sending messages is not supported'));
  }

  /**
   * @override
   */
  sendRouteBinaryMessage(routeId, message) {
    return Promise.reject(Error('DIAL sending messages is not supported'));
  }

  /**
   * @override
   */
  canRoute(sourceUrn, sinkId) {
    const sink = this.sinkDiscoveryService_.getSinkById(sinkId);
    if (!sink) {
      return false;
    }

    if (MediaSourceUtils.isMirrorSource(sourceUrn)) {
      return false;
    }

    const dialMediaSource = DialPresentationUrl.create(sourceUrn);
    if (!dialMediaSource) {
      return false;
    }
    return sink.getAppStatus(dialMediaSource.appName) ==
        SinkAppStatus.AVAILABLE;
  }

  /**
   * @override
   */
  canJoin(sourceUrn, presentationId, route) {
    return false;
  }

  /**
   * @override
   */
  searchSinks(sourceUrn, searchCriteria) {
    // Not implemented.
    return Assertions.rejectNotImplemented();
  }

  /**
   * @override
   */
  createMediaRouteController(routeId, controllerRequest, observer) {
    // Not implemented.
    return Assertions.rejectNotImplemented();
  }

  /**
   * @override
   */
  provideSinks(sinks) {
    this.sinkDiscoveryService_.addSinks(sinks);
  }

  /**
   * @param {!DialSink} sink
   * @return {!DialClient.Client}
   * @private
   */
  newClient_(sink) {
    return new DialClient.Client(sink);
  }
};

exports = DialProvider;
