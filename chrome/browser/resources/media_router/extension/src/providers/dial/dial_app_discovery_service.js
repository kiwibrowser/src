// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.module('mr.dial.AppDiscoveryService');

const ActivityRecords = goog.require('mr.dial.ActivityRecords');
const DialClient = goog.require('mr.dial.Client');
const DialSink = goog.require('mr.dial.Sink');
const DialSinkAppStatus = goog.require('mr.dial.SinkAppStatus');
const Logger = goog.require('mr.Logger');
const PersistentData = goog.require('mr.PersistentData');
const PersistentDataManager = goog.require('mr.PersistentDataManager');
const PromiseUtils = goog.require('mr.PromiseUtils');
const SinkDiscoveryService = goog.require('mr.dial.SinkDiscoveryService');


/**
 * Service for determining whether a sink supports a given DIAL app.
 * @implements {PersistentData}
 */
const AppDiscoveryService = class {
  /**
   * @param {!SinkDiscoveryService} discoveryService
   * @param {!ActivityRecords} activityRecords
   */
  constructor(discoveryService, activityRecords) {
    /** @private @const {?Logger} */
    this.logger_ = Logger.getInstance('mr.dial.AppDiscoveryService');

    /**
     * Names of applications that are actively being queried for.
     * @private {!Set<string>}
     */
    this.registeredApps_ = new Set();

    /**
     * @private @const {!SinkDiscoveryService}
     */
    this.discoveryService_ = discoveryService;

    /**
     * @private @const {!ActivityRecords}
     */
    this.activityRecords_ = activityRecords;

    /**
     * Keeps track of pending requests to avoid duplication.  Values are
     * Promises returning strings of the form <sink.getId()>:<appName>.
     * @private @const {!Map<string, !Promise<!DialClient.AppInfo>>}
     */
    this.pendingRequests_ = new Map();

    /**
     * DIAL clients used by this service.  Keys are sink ids.
     * @private @const {!Map<string, !DialClient.Client>}
     */
    this.clientsBySinkId_ = new Map();

    /**
     * The timeout ID for the next scan.
     * @private {?number}
     */
    this.timeoutId_ = null;

    /**
     * Whether the service is running.
     * @private {boolean}
     */
    this.running_ = false;
  }

  init() {
    PersistentDataManager.register(this);
  }

  /**
   * Starts discovery.
   */
  start() {
    this.logger_.info('Starting periodic scanning.');
    if (!this.running_) {
      this.running_ = true;
      this.doScan_();
    }
  }

  /**
   * Stops discovery. Also aborts all outstanding discovery requests.
   */
  stop() {
    this.logger_.info('Stopping periodic scanning.');
    if (!this.running_) return;
    this.running_ = false;
    if (this.timeoutId_) {
      clearTimeout(this.timeoutId_);
      this.timeoutId_ = null;
    }
    this.pendingRequests_.clear();
  }

  /**
   * Registers an application name that should be queried. Starts periodic
   * scanning if it hasn't already started. Otherwise, issues an out-of-band
   * query for the app against all discovered sinks.
   * @param {string} appName
   */
  registerApp(appName) {
    if (this.registeredApps_.has(appName) &&
        !this.anyUnknownAppStatus_(appName)) {
      // No need to scan since status for appName is known for all sinks.
      return;
    }

    this.registeredApps_.add(appName);

    if (this.discoveryService_.getSinkCount() == 0) {
      // No sinks, no need to scan.
      return;
    }
    if (!this.running_) {
      this.start();
    } else {
      this.discoveryService_.getSinks().forEach(
          this.doScanSinkForApp_.bind(this, appName));
    }
  }

  /**
   * Unregisters an application name.
   * @param {string} appName
   */
  unregisterApp(appName) {
    this.registeredApps_.delete(appName);
  }

  /**
   * @return {!Array<string>}
   */
  getRegisteredApps() {
    return Array.from(this.registeredApps_);
  }

  /**
   * @return {number}
   */
  getAppCount() {
    return this.registeredApps_.size;
  }

  /**
   * Issues app info queries and updates sink app status and activities
   * according to results. Queries will be made for the following:
   * 1) All registered apps, against all discovered sinks. Note that a (app,
   * sink) combination is only queried if its status is unknown, or if enough
   * time has elapsed since its previous status was known.
   * 2) Each activity's app against its sink.
   * Once all queries have returned and all updates have been made, schedules a
   * scan in the future.
   * @private
   */
  doScan_() {
    this.logger_.info('Start app status scan.');
    const promises = [];

    // Scans all sinks against all registered apps.
    this.discoveryService_.getSinks().forEach(sink => {
      promises.push.apply(promises, this.scanSink(sink));
    });

    // Scans all activities.
    this.activityRecords_.getActivities().forEach(activity => {
      promises.push(this.scanActivity_(activity));
    });

    PromiseUtils.allSettled(promises).then(() => {
      if (this.running_ && !this.timeoutId_) {
        this.logger_.fine('Scan complete; scheduling for next scan.');
        // NOTE(imcheng): setTimeout with a large delay does not work well in
        // event pages. Instead we should be using chrome.alarms API, but note
        // that it is subject to a minmum delay of 1 minute.
        this.timeoutId_ = setTimeout(() => {
          this.doRescan_();
        }, AppDiscoveryService.CHECK_INTERVAL_MILLIS);
      }
    });
  }

  /**
   * Clears the timer and starts a round of scanning. Only valid when called
   * from a timer.
   * @private
   */
  doRescan_() {
    this.logger_.fine('Start app status scan (timer-based)');
    this.timeoutId_ = null;
    this.doScan_();
  }

  /**
   * Asynchronously scans a sink against all registered apps and updates its app
   * status map if needed.
   * @param {!DialSink} sink
   * @return {!Array<!Promise<void>>} If the sink does not support app
   *     the getting app info, returns an empty array. Otherwise, returns a list
   *     of Promises, each corresponding to a registered app, resolved when the
   *     sink's app status has been updated.
   */
  scanSink(sink) {
    if (!sink.supportsAppAvailability()) {
      return [];
    }

    const promises = [];
    for (let appName of this.registeredApps_) {
      promises.push(this.doScanSinkForApp_(appName, sink));
    }
    return promises;
  }

  /**
   * Issues an app info query for the given app to the given sink, and updates
   * the sink's app status map with the response.
   * @param {string} appName
   * @param {!DialSink} sink
   * @return {!Promise<void>} Resolved when the sink's app status has been
   *     updated.
   * @private
   */
  doScanSinkForApp_(appName, sink) {
    if (sink.getAppStatus(appName) != DialSinkAppStatus.UNKNOWN &&
        Date.now() - sink.getAppStatusTimeStamp(appName) <
            AppDiscoveryService.CACHE_PERIOD_) {
      // App status already known and not expired.
      return Promise.resolve();
    }
    if (!sink.supportsAppAvailability()) {
      return Promise.resolve();
    }
    this.logger_.fine(
        'Querying ' + sink.getId() + ' for ' + appName +
        ' to update app status');
    return this.getAppInfo_(sink, appName)
        .then(
            appInfo => {
              const newAppStatus = this.getAvailabilityFromAppInfo_(appInfo);
              this.maybeUpdateAppStatus_(sink, appName, newAppStatus);
            },
            e => {
              // Some devices return NOT_FOUND for GetAppInfo to indicate the
              // app is unavailable.
              if (e instanceof DialClient.AppInfoNotFoundError) {
                this.maybeUpdateAppStatus_(
                    sink, appName, DialSinkAppStatus.UNAVAILABLE);
                return;
              }
              this.logger_.warning(
                  'Failed to process app availability; ' + sink.getId() +
                  ' does not support app availability');
              sink.setSupportsAppAvailability(false);
            });
  }

  /**
   * Issues an app info query for the given activity's app to the activity's
   * sink, and updates the activity records if the app is no longer running.
   * @param {!mr.dial.Activity} activity
   * @return {!Promise<void>} Resolved when the activity record has possibly
   *     been updated.
   * @private
   */
  scanActivity_(activity) {
    const sink = this.discoveryService_.getSinkById(activity.route.sinkId);
    if (!sink) {
      this.logger_.warning(
          'Activity refers to nonexistent sink: ' + activity.route.id);
      return Promise.resolve();
    }
    if (!sink.supportsAppAvailability()) {
      return Promise.resolve();
    }
    const appName = activity.appName;
    this.logger_.fine(
        'Querying ' + sink.getId() + ' for ' + appName + ' to update activity');
    return this.getAppInfo_(sink, appName)
        .then(
            appInfo => this.maybeUpdateActivityRecord_(
                /** @type {!DialSink} */ (sink), appName, appInfo),
            e => this.doRemoveActivityRecord_(
                /** @type {!DialSink} */ (sink), appName));
  }

  /**
   * Gets the DIAL client associated with the given sink, or creates one if it
   * does not exist.
   * @param {!DialSink} sink
   * @return {!DialClient.Client} A client instance for the given sink.
   * @private
   */
  getDialClient_(sink) {
    let client = this.clientsBySinkId_.get(sink.getId());
    if (!client) {
      client = new DialClient.Client(sink);
      this.logger_.fine('Created DIAL client for ' + sink.getId());
      this.clientsBySinkId_.set(sink.getId(), client);
    }
    return client;
  }

  /**
   * Issues an app info query with the given sink's DIAL client. A query will
   * not be issued if there is already a same one pending.
   * @param {!DialSink} sink
   * @param {string} appName
   * @return {!Promise<!DialClient.AppInfo>} Resolved with the query response,
   *     or rejected on error.
   * @private
   */
  getAppInfo_(sink, appName) {
    const requestId = AppDiscoveryService.getRequestId_(sink, appName);
    let promise = this.pendingRequests_.get(requestId);
    if (promise) {
      return promise;
    }

    promise = this.getDialClient_(sink).getAppInfo(appName);
    this.pendingRequests_.set(requestId, promise);
    const cleanup = () => {
      this.pendingRequests_.delete(requestId);
    };
    promise.then(cleanup, cleanup);
    return promise;
  }

  /**
   * Translates the given app info result into a DialSinkAppStatus value.
   * @param {!DialClient.AppInfo} appInfo
   * @return {DialSinkAppStatus}
   * @private
   */
  getAvailabilityFromAppInfo_(appInfo) {
    if (appInfo.name == 'Netflix') {
      return this.getAppStatusFromNetflixAppInfo_(appInfo);
    } else {
      switch (appInfo.state) {
        case DialClient.DialAppState.RUNNING:
        case DialClient.DialAppState.STOPPED:
          return DialSinkAppStatus.AVAILABLE;
        default:
          return DialSinkAppStatus.UNAVAILABLE;
      }
    }
  }

  /**
   * Returns app status from a Netflix app info.
   * @param {!DialClient.AppInfo} appInfo The app info from DIAL GET.
   * @return {DialSinkAppStatus}
   * @private
   */
  getAppStatusFromNetflixAppInfo_(appInfo) {
    const isNetflixWebsocket =
        appInfo.extraData && appInfo.extraData['capabilities'] == 'websocket';
    if (isNetflixWebsocket &&
        (appInfo.state == DialClient.DialAppState.RUNNING ||
         appInfo.state == DialClient.DialAppState.STOPPED)) {
      return DialSinkAppStatus.AVAILABLE;
    } else {
      return DialSinkAppStatus.UNAVAILABLE;
    }
  }

  /**
   * Returns the request ID to use for an app info request.
   * @param {!DialSink} sink
   * @param {string} appName
   * @return {string}
   * @private
   */
  static getRequestId_(sink, appName) {
    return sink.getId() + ':' + appName;
  }

  /**
   * Checks whether the status of an app on a sink has changed, and if so
   * notifies discovery service.
   * @param {!DialSink} sink
   * @param {string} appName
   * @param {DialSinkAppStatus} newAppStatus
   * @private
   */
  maybeUpdateAppStatus_(sink, appName, newAppStatus) {
    this.logger_.fine(
        'Got app status ' + newAppStatus + ' from ' + sink.getId() + ' for ' +
        appName);
    const oldAppStatus = sink.getAppStatus(appName);
    sink.setAppStatus(appName, newAppStatus);
    if (newAppStatus != oldAppStatus) {
      this.discoveryService_.onAppStatusChanged(appName, sink);
    }
  }

  /**
   * Checks whether the given app is no longer running on the given sink, and if
   * so notifies activity records.
   * @param {!DialSink} sink
   * @param {string} appName
   * @param {!DialClient.AppInfo} appInfo
   * @private
   */
  maybeUpdateActivityRecord_(sink, appName, appInfo) {
    if (appInfo.state != DialClient.DialAppState.RUNNING) {
      this.doRemoveActivityRecord_(sink, appName);
    }
  }

  /**
   * Removes the activity record with the given sink and app name, if it exists.
   * @param {!DialSink} sink
   * @param {string} appName
   * @private
   */
  doRemoveActivityRecord_(sink, appName) {
    const activity = this.activityRecords_.getBySinkId(sink.getId());
    if (activity && activity.appName == appName) {
      this.activityRecords_.removeByRouteId(activity.route.id);
    }
  }

  /**
   * Checks if there is a sink whose status of appName is unknown.
   * @param {string} appName
   * @return {boolean}
   * @private
   */
  anyUnknownAppStatus_(appName) {
    return this.discoveryService_.getSinks().some(
        s => s.getAppStatus(appName) == DialSinkAppStatus.UNKNOWN);
  }

  /**
   * @override
   */
  getStorageKey() {
    return 'dial.AppDiscoveryService';
  }

  /**
   * @override
   */
  getData() {
    return [this.getRegisteredApps()];
  }

  /**
   * @override
   */
  loadSavedData() {
    const savedData = PersistentDataManager.getTemporaryData(this);
    this.registeredApps_ = new Set(savedData || []);
  }
};


/**
 * The interval for periodic scanning.
 * @package @const {number}
 */
AppDiscoveryService.CHECK_INTERVAL_MILLIS = 60 * 1000;


/**
 * The amount of time an availability result for an app (both AVAILABLE and
 * UNAVAILABLE) will be cached for.
 * @private @const {number}
 */
AppDiscoveryService.CACHE_PERIOD_ = 60 * 60 * 1000;


exports = AppDiscoveryService;
