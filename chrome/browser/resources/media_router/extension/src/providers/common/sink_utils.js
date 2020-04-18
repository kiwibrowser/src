// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Utilites and settings for media sinks.

 */

goog.module('mr.SinkUtils');
goog.module.declareLegacyNamespace();

const PersistentData = goog.require('mr.PersistentData');
const PersistentDataManager = goog.require('mr.PersistentDataManager');
const Sha1 = goog.require('mr.Sha1');
const StringUtils = goog.require('mr.StringUtils');
const base64 = goog.require('mr.base64');


/**
 * @implements {PersistentData}
 */
class SinkUtils {
  constructor() {
    /**
     * Used to generate per-profile media sink ids.  Persistent data.
     * @private {string}
     */
    this.receiverIdToken_ = StringUtils.getRandomString();

    /**
     * The model name of the device that was most recently used to start a media
     * route. Temporary data.
     * @type {!SinkUtils.DeviceData}
     */
    this.recentLaunchedDevice = new SinkUtils.DeviceData(null, null);

    /**
     * The model name of the device that was most recently discovered. Temporary
     * data.
     * @type {!SinkUtils.DeviceData}
     */
    this.recentDiscoveredDevice = new SinkUtils.DeviceData(null, null);

    /**
     * List of IP addresses of devices that are assumed to exist (and not
     * discvered on the LAN), for debugging purposes.  Persistent data.
     * @type {!Array<string>}
     */
    this.fixedIpList = [];

    /**
     * Persistent data.
     * @type {number}
     */
    this.castControlPort = 0;

    /**
     * The last time cloud sinks were checked.
     * @type {number}
     */
    this.lastCloudSinkCheckTimeMillis = 0;

    PersistentDataManager.register(this);
  }

  /**
   * @return {!SinkUtils}
   */
  static getInstance() {
    if (!SinkUtils.instance_) {
      SinkUtils.instance_ = new SinkUtils();
    }
    return SinkUtils.instance_;
  }

  /**
   * Generates ID from the receiver UUID and a per-profile token saved in
   * localStorage.
   *
   * Both DIAL and mDNS use this to generate receiver ID so that it is
   * consistent and can be used to deduplicate receivers. For a given token, the
   * ID is the same for the same device no matter when it is discovered.
   *
   * @param {string} uniqueId
   * @return {string} receiver ID.
   */
  generateId(uniqueId) {
    uniqueId = uniqueId.toLowerCase();
    const sha1 = new Sha1();
    sha1.update(uniqueId);
    sha1.update(this.receiverIdToken_);
    return 'r' + base64.encodeArray(sha1.digest(), true);
  }

  /**
   * @return {!SinkUtils.DeviceData} Most recent device launched or
   * discovered.
   */
  getRecentDevice() {
    if (this.recentLaunchedDevice.model) return this.recentLaunchedDevice;

    if (this.recentDiscoveredDevice.model) return this.recentDiscoveredDevice;

    return new SinkUtils.DeviceData(null, null);
  }

  /**
   * @override
   */
  getStorageKey() {
    return 'SinkUtils';
  }

  /**
   * @override
   */
  getData() {
    return [
      {
        'recentLaunchedDevice': this.recentLaunchedDevice,
        'recentDiscoveredDevice': this.recentDiscoveredDevice
      },
      {
        'receiverIdToken': this.receiverIdToken_,
        'fixedIpList': this.fixedIpList.join(','),
        'castControlPort': this.castControlPort,
        'lastCloudSinkCheckTimeMillis': this.lastCloudSinkCheckTimeMillis
      }
    ];
  }

  /**
   * @override
   */
  loadSavedData() {
    const tempData = PersistentDataManager.getTemporaryData(this);
    if (tempData) {
      this.recentLaunchedDevice = tempData['recentLaunchedDevice'] ||
          new SinkUtils.DeviceData(null, null);
      this.recentDiscoveredDevice = tempData['recentDiscoveredDevice'] ||
          new SinkUtils.DeviceData(null, null);
    }

    const persistentData = PersistentDataManager.getPersistentData(this);
    if (persistentData) {
      this.receiverIdToken_ =
          persistentData['receiverIdToken'] || StringUtils.getRandomString();
      this.fixedIpList = (persistentData['fixedIpList'] &&
                          persistentData['fixedIpList'].split(',')) ||
          [];
      this.castControlPort = persistentData['castControlPort'] || 0;
      this.lastCloudSinkCheckTimeMillis =
          persistentData['lastCloudSinkCheckTimeMillis'] || 0;
    }
  }
}


/** @private {SinkUtils} */
SinkUtils.instance_ = null;


/**
 * The device data to keep track of.
 */
SinkUtils.DeviceData = class {
  /**
   * @param {?string} modelName
   * @param {?string} ipAddress
   */
  constructor(modelName, ipAddress) {
    /**
     * @type {?string}
     * @export
     */
    this.model = modelName;

    /**
     * @type {?string}
     * @export
     */
    this.ip = ipAddress;
  }
};

exports = SinkUtils;
