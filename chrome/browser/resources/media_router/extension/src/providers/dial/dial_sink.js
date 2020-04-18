// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.module('mr.dial.Sink');
goog.module.declareLegacyNamespace();

const Sink = goog.require('mr.Sink');
const SinkAppStatus = goog.require('mr.dial.SinkAppStatus');


/**
 * A wrapper for Sink containing DIAL specific data.
 */
const DialSink = class {
  /**
   * @param {string} friendlyName
   * @param {string} uniqueId
   * @final
   */
  constructor(friendlyName, uniqueId) {
    /** @private {?string} */
    this.ipAddress_ = null;

    /** @private {?number} */
    this.port_ = null;

    /** @private {?string} */
    this.dialAppUrl_ = null;

    /** @private {?string} */
    this.deviceDescriptionUrl_ = null;

    /** @private {?string} */
    this.modelName_ = null;

    /** @private @const {!Sink} */
    this.mrSink_ = new Sink(uniqueId, friendlyName);

    /**
     * Holds the status of applications that may be available on the sink.
     * Keys are application Names.
     * @private {!Object<string, SinkAppStatus>}
     */
    this.appStatusMap_ = {};

    /**
     * Holds the timestamp when the status of applications was set.
     * @private {!Object<string, number>}
     */
    this.appStatusTimeStamp_ = {};

    /** @private {boolean} */
    this.supportsAppAvailability_ = false;
  }

  /**
   * @return {!Sink}
   */
  getMrSink() {
    return this.mrSink_;
  }

  /**
   * @return {string} A human readable name for the sink.
   */
  getFriendlyName() {
    return this.mrSink_.friendlyName;
  }

  /**
   * @param {string} friendlyName
   * @return {!mr.dial.Sink} This sink.
   */
  setFriendlyName(friendlyName) {
    this.mrSink_.friendlyName = friendlyName;
    return this;
  }

  /**
   * @return {?string} sink model name if known.
   */
  getModelName() {
    return this.modelName_;
  }

  /**
   * @param {?string} modelName
   * @return {!mr.dial.Sink} This sink.
   */
  setModelName(modelName) {
    this.modelName_ = modelName;
    return this;
  }

  /**
   * @return {string} An identifier for this sink.
   */
  getId() {
    return this.mrSink_.id;
  }

  /**
   * @param {string} id
   * @return {!mr.dial.Sink} This sink.
   */
  setId(id) {
    this.mrSink_.id = id;
    return this;
  }

  /**
   * @return {boolean} Whether this sink supports queries for DIAL app
   *     availability.
   */
  supportsAppAvailability() {
    return this.supportsAppAvailability_;
  }

  /**
   * Sets whether this sink supports DIAL app availability queries.
   * @param {boolean} availability
   * @return {!mr.dial.Sink} This sink.
   */
  setSupportsAppAvailability(availability) {
    this.supportsAppAvailability_ = availability;
    return this;
  }

  /**
   * Updates sink properties.
   * Fields that can be updated: friendlyName, dialAppUrl_,
   * deviceDescriptionUrl_, ipAddress_, port_.
   * @param {!mr.dial.Sink} sink
   * @return {boolean} Whether the update resulted in changes to the sink.
   */
  update(sink) {
    if (this.getId() != sink.getId()) {
      return false;
    }

    let updated = false;

    if (this.mrSink_.friendlyName != sink.mrSink_.friendlyName) {
      this.mrSink_.friendlyName = sink.mrSink_.friendlyName;
      updated = true;
    }

    if (this.dialAppUrl_ != sink.dialAppUrl_) {
      this.dialAppUrl_ = sink.dialAppUrl_;
      updated = true;
    }

    if (this.deviceDescriptionUrl_ != sink.deviceDescriptionUrl_) {
      this.deviceDescriptionUrl_ = sink.deviceDescriptionUrl_;
      updated = true;
    }

    if (this.ipAddress_ != sink.ipAddress_) {
      this.ipAddress_ = sink.ipAddress_;
      updated = true;
    }

    if (this.port_ != sink.port_) {
      this.port_ = sink.port_;
      updated = true;
    }

    return updated;
  }

  /**
   * @return {?string} The IP address of the sink, if any.
   */
  getIpAddress() {
    return this.ipAddress_;
  }

  /**
   * @param {?string} ipAddress The sink IP address.
   * @return {!mr.dial.Sink} This sink.
   */
  setIpAddress(ipAddress) {
    this.ipAddress_ = ipAddress;
    return this;
  }

  /**
   * @return {?number} The port number of the secure channel service.
   */
  getPort() {
    return this.port_;
  }

  /**
   * @param {?number} port
   * @return {!mr.dial.Sink} This sink.
   */
  setPort(port) {
    this.port_ = port;
    return this;
  }

  /**
   * @return {?string} The DIAL application URL, if any.
   */
  getDialAppUrl() {
    return this.dialAppUrl_;
  }

  /**
   * @param {string} url The DIAL app URL.
   * @return {!mr.dial.Sink} This sink.
   */
  setDialAppUrl(url) {
    this.dialAppUrl_ = url;
    return this;
  }

  /**
   * @return {?string} The DIAL device description URL, if any.
   */
  getDeviceDescriptionUrl() {
    return this.deviceDescriptionUrl_;
  }

  /**
   * @param {string} url The DIAL device description URL.
   * @return {!mr.dial.Sink} This sink.
   */
  setDeviceDescriptionUrl(url) {
    this.deviceDescriptionUrl_ = url;
    return this;
  }

  /**
   * Gets the availability of an application.
   * @param {string} appName
   * @return {SinkAppStatus} The status of the application, or null if it was
   *     not set.
   */
  getAppStatus(appName) {
    return this.appStatusMap_[appName] || SinkAppStatus.UNKNOWN;
  }

  /**
   * Gets the time stamp of the availability of an application was set.
   * @param {string} appName
   * @return {?number} the number of milliseconds between midnight, January 1,
   *     1970 and the current time, or null if availability was not set.
   */
  getAppStatusTimeStamp(appName) {
    return this.appStatusTimeStamp_[appName] || null;
  }

  /**
   * Sets the availability of an application.
   * @param {string} appName
   * @param {SinkAppStatus} status
   * @return {!mr.dial.Sink} This sink.
   */
  setAppStatus(appName, status) {
    this.appStatusMap_[appName] = status;
    this.appStatusTimeStamp_[appName] = Date.now();
    return this;
  }

  /**
   * Clears all app status from the sink.
   * @return {!mr.dial.Sink} This sink.
   */
  clearAppStatus() {
    this.appStatusMap_ = {};
    this.appStatusTimeStamp_ = {};
    return this;
  }

  /**
   * @return {string} String suitable for fine logging.
   */
  toDebugString() {
    return 'name = ' + this.mrSink_.friendlyName +
        (this.ipAddress_ ? ', ip = ' + this.ipAddress_ : '') +
        (this.modelName_ ? ', model = ' + this.modelName_ : '') +
        ', apps = ' + JSON.stringify(this.appStatusMap_);
  }

  /**
   * Creates a new sink and copies the fields of the input sink to this.
   * @param {!Object<string, *>} sink The object containing data fields.
   * @return {!mr.dial.Sink} A newly created sink.
   */
  static createFrom(sink) {
    const newSink = new DialSink(sink.mrSink_.friendlyName, '');
    newSink.mrSink_.id = sink.mrSink_.id;  // Override the sink ID.
    newSink.ipAddress_ = sink.ipAddress_;
    newSink.port_ = sink.port_;
    newSink.dialAppUrl_ = sink.dialAppUrl_;
    newSink.deviceDescriptionUrl_ = sink.deviceDescriptionUrl_;
    newSink.modelName_ = sink.modelName_;
    newSink.appStatusMap_ = sink.appStatusMap_;
    newSink.appStatusTimeStamp_ = sink.appStatusTimeStamp_;
    newSink.supportsAppAvailability_ = sink.supportsAppAvailability_;
    return newSink;
  }
};


exports = DialSink;
