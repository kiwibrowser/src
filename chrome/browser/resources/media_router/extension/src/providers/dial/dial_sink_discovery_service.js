// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.module('mr.dial.SinkDiscoveryService');
goog.module.declareLegacyNamespace();

const DeviceCounts = goog.require('mr.DeviceCounts');
const DeviceCountsProvider = goog.require('mr.DeviceCountsProvider');
const DialAnalytics = goog.require('mr.DialAnalytics');
const DialSink = goog.require('mr.dial.Sink');
const Logger = goog.require('mr.Logger');
const PersistentData = goog.require('mr.PersistentData');
const PersistentDataManager = goog.require('mr.PersistentDataManager');
const SinkAppStatus = goog.require('mr.dial.SinkAppStatus');
const SinkDiscoveryCallbacks = goog.require('mr.dial.SinkDiscoveryCallbacks');
const SinkList = goog.require('mr.SinkList');


/**
 * Implements local discovery using DIAL.
 * DIAL specification:
 * http://www.dial-multiscreen.org/dial-protocol-specification
 * @implements {PersistentData}
 * @implements {DeviceCountsProvider}
 */
class SinkDiscoveryService {
  /**
   * @param {!SinkDiscoveryCallbacks} sinkCallBacks
   * @final
   */
  constructor(sinkCallBacks) {
    /**
     * @private @const {!SinkDiscoveryCallbacks}
     */
    this.sinkCallBacks_ = sinkCallBacks;

    /**
     * @private @const {?Logger}
     */
    this.logger_ = Logger.getInstance('mr.dial.SinkDiscoveryService');

    /**
     * The current set of *accessible* receivers, indexed by id.
     * @private @const {!Map<string, !DialSink>}
     */
    this.sinkMap_ = new Map();

    /**
     * The most recent snapshot of device counts.
     * Updated when a DIAL onDeviceList or onError event is received.
     * Part of PersistentData.
     * @private {!DeviceCounts}
     */
    this.deviceCounts_ = {availableDeviceCount: 0, knownDeviceCount: 0};

    /**
     * The last time device counts were recorded in DialAnalytics.
     * Persistent data.
     * @private {number}
     */
    this.deviceCountMetricsRecordTime_ = 0;
  }

  /**
   * Initializes the service. Must be called before any other methods.
   */
  init() {
    PersistentDataManager.register(this);
  }

  /**
   * Add |sinks| to sink map. Remove outdated sinks that are in sink map but not
   * in |sinks|.
   * @param {!Array<!mojo.Sink>} sinks list of sinks discovered by Media Router.
   */
  addSinks(sinks) {
    this.logger_.info('addSinks returned ' + sinks.length + ' sinks');
    this.logger_.fine(() => '....the list is: ' + JSON.stringify(sinks));

    const oldSinkIds = new Set(this.sinkMap_.keys());
    sinks.forEach(mojoSink => {
      const dialSink = SinkDiscoveryService.convertSink_(mojoSink);
      this.mayAddSink_(dialSink);
      oldSinkIds.delete(dialSink.getId());
    });

    let removedSinks = [];
    oldSinkIds.forEach(sinkId => {
      const sink = this.sinkMap_.get(sinkId);
      removedSinks.push(sink);
      this.sinkMap_.delete(sinkId);
    });

    if (removedSinks.length > 0) {
      this.sinkCallBacks_.onSinksRemoved(removedSinks);
    }

    // Record device count for feedback.
    const sinkCount = this.getSinkCount();
    this.deviceCounts_ = {
      availableDeviceCount: sinkCount,
      knownDeviceCount: sinkCount
    };
  }

  /**
   * Updates deviceCounts_ with the given counts, and reports to analytics if
   * applicable.
   * @param {number} availableDeviceCount
   * @param {number} knownDeviceCount
   * @private
   */
  recordDeviceCounts_(availableDeviceCount, knownDeviceCount) {
    this.deviceCounts_ = {
      availableDeviceCount: availableDeviceCount,
      knownDeviceCount: knownDeviceCount
    };
    if (Date.now() - this.deviceCountMetricsRecordTime_ <
        SinkDiscoveryService.DEVICE_COUNT_METRIC_THRESHOLD_MS_) {
      return;
    }
    DialAnalytics.recordDeviceCounts(this.deviceCounts_);
    this.deviceCountMetricsRecordTime_ = Date.now();
  }

  /**
   * Adds or updates an existing sink with the given sink.
   * @param {!DialSink} sink The new or updated sink.
   * @private
   */
  mayAddSink_(sink) {
    this.logger_.fine('mayAddSink, id = ' + sink.getId());
    const sinkToUpdate = this.sinkMap_.get(sink.getId());
    if (sinkToUpdate) {
      if (sinkToUpdate.update(sink)) {
        this.logger_.fine('Updated sink ' + sinkToUpdate.getId());
        this.sinkCallBacks_.onSinkUpdated(sinkToUpdate);
      }
    } else {
      this.logger_.fine(
          () => `Adding new sink ${sink.getId()}: ${sink.toDebugString()}`);
      this.sinkMap_.set(sink.getId(), sink);
      this.sinkCallBacks_.onSinkAdded(sink);
    }
  }

  /**
   * Converts a mojo.Sink to a DialSink.
   * @param {!mojo.Sink} mojoSink returned by Media Router at browser side.
   * @return {!DialSink} DIAL sink.
   * @private
   */
  static convertSink_(mojoSink) {

    const uniqueId = mojoSink.sink_id;
    const extraData = mojoSink.extra_data.dial_media_sink;
    const isDiscoveryOnly =
        SinkDiscoveryService.isDiscoveryOnly_(extraData.model_name);

    const ip_address = extraData.ip_address.address_bytes ?
        extraData.ip_address.address_bytes.join('.') :
        extraData.ip_address.address.join('.');
    return new DialSink(mojoSink.name, uniqueId)
        .setIpAddress(ip_address)
        .setDialAppUrl(extraData.app_url.url)
        .setModelName(extraData.model_name)
        .setSupportsAppAvailability(!isDiscoveryOnly);
  }

  /**
   * Returns true if DIAL (SSDP) was only used to discover this sink, and it is
   * not expected to support other DIAL features (app discovery, activity
   * discovery, etc.)
   * @param {string} modelName
   * @return {boolean}
   * @private
   */
  static isDiscoveryOnly_(modelName) {
    return SinkDiscoveryService.DISCOVERY_ONLY_RE_.test(modelName);
  }

  /**
   * Returns the sink with the given ID, or null if not found.
   * @param {string} sinkId
   * @return {?DialSink}
   */
  getSinkById(sinkId) {
    return this.sinkMap_.get(sinkId) || null;
  }

  /**
   * Returns sinks that report availability of the given app name.
   * @param {string} appName
   * @return {!SinkList}
   */
  getSinksByAppName(appName) {
    const sinks = [];
    this.sinkMap_.forEach(dialSink => {
      if (dialSink.getAppStatus(appName) == SinkAppStatus.AVAILABLE)
        sinks.push(dialSink.getMrSink());
    });
    return new SinkList(
        sinks, SinkDiscoveryService.APP_ORIGIN_WHITELIST_[appName]);
  }

  /**
   * Returns current sinks.
   * @return {!Array<!DialSink>}
   */
  getSinks() {
    return Array.from(this.sinkMap_.values());
  }

  /**
   * @override
   */
  getDeviceCounts() {
    return this.deviceCounts_;
  }

  /**
   * @return {number}
   */
  getSinkCount() {
    return this.sinkMap_.size;
  }

  /**
   * Invoked when the app status of a sink changes.
   * @param {string} appName
   * @param {!DialSink} sink The sink whose status changed.
   */
  onAppStatusChanged(appName, sink) {
    this.sinkCallBacks_.onSinkUpdated(sink);
  }

  /**
   * @override
   */
  getStorageKey() {
    return 'dial.DialSinkDiscoveryService';
  }

  /**
   * @override
   */
  getData() {
    return [
      new SinkDiscoveryService.PersistentData_(
          Array.from(this.sinkMap_), this.deviceCounts_),
      {'deviceCountMetricsRecordTime': this.deviceCountMetricsRecordTime_}
    ];
  }

  /**
   * @override
   */
  loadSavedData() {
    const tempData =
        /** @type {?SinkDiscoveryService.PersistentData_} */ (
            PersistentDataManager.getTemporaryData(this));
    if (tempData) {
      for (const entry of tempData.sinks) {
        this.sinkMap_.set(entry[0], DialSink.createFrom(entry[1]));
      }
      this.deviceCounts_ = tempData.deviceCounts;
    }

    const permanentData = PersistentDataManager.getPersistentData(this);
    if (permanentData) {
      this.deviceCountMetricsRecordTime_ =
          permanentData['deviceCountMetricsRecordTime'];
    }
  }
}


/**
 * @private @const {!Object<string, !Array<string>>}
 */
SinkDiscoveryService.APP_ORIGIN_WHITELIST_ = {
  'YouTube': [
    'https://tv.youtube.com', 'https://tv-green-qa.youtube.com',
    'https://tv-release-qa.youtube.com', 'https://web-green-qa.youtube.com',
    'https://web-release-qa.youtube.com', 'https://www.youtube.com'
  ],
  'Netflix': ['https://www.netflix.com'],
  'Pandora': ['https://www.pandora.com'],
  'Radio': ['https://www.pandora.com'],
  'Hulu': ['https://www.hulu.com'],
  'Vimeo': ['https://www.vimeo.com'],
  'Dailymotion': ['https://www.dailymotion.com'],
  'com.dailymotion': ['https://www.dailymotion.com'],
};


/**
 * Matches DIAL model names that only support discovery.

 * @private @const {!RegExp}
 */
SinkDiscoveryService.DISCOVERY_ONLY_RE_ =
    new RegExp('Eureka Dongle|Chromecast Audio|Chromecast Ultra', 'i');

/**
 * How long to wait between device counts metrics are recorded. Set to 1 hour.
 * @private @const {number}
 */
SinkDiscoveryService.DEVICE_COUNT_METRIC_THRESHOLD_MS_ = 60 * 60 * 1000;


/**
 * @private
 */
SinkDiscoveryService.PersistentData_ = class {
  /**
   * @param {!Array} sinks
   * @param {!DeviceCounts} deviceCounts
   */
  constructor(sinks, deviceCounts) {
    /**
     * @const {!Array}
     */
    this.sinks = sinks;

    /**
     * @const {!DeviceCounts}
     */
    this.deviceCounts = deviceCounts;
  }
};

exports = SinkDiscoveryService;
