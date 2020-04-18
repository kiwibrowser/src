// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.module('mr.dial.ActivityRecords');

const ActivityCallbacks = goog.require('mr.dial.ActivityCallbacks');
const PersistentData = goog.require('mr.PersistentData');
const PersistentDataManager = goog.require('mr.PersistentDataManager');


/**
 * Keeps track of DIAL activities.
 * @implements {PersistentData}
 */
const ActivityRecords = class {
  /**
   * @param {!ActivityCallbacks} activityCallbacks
   */
  constructor(activityCallbacks) {
    /**
     * Maps route ID to the corresponding Activity.
     * @private {!Map<string, !mr.dial.Activity>}
     */
    this.routeIdToAppInfo_ = new Map();

    /** @private @const {!ActivityCallbacks} */
    this.activityCallbacks_ = activityCallbacks;
  }

  /**
   * Restores routeIdToAppInfo_ from saved data.
   */
  init() {
    PersistentDataManager.register(this);
  }

  /**
   * Removes all activities.
   */
  clear() {
    this.routeIdToAppInfo_.clear();
  }

  /**
   * Adds a new activity. If the activity already exists, this is no-op.
   * @param {!mr.dial.Activity} activity
   */
  add(activity) {
    if (this.getByRouteId(activity.route.id)) {
      return;
    }
    this.routeIdToAppInfo_.set(activity.route.id, activity);
    this.activityCallbacks_.onActivityAdded(activity);
  }

  /**
   * Returns the activity corresponding to the given route ID, or null if there
   * is none.
   * @param {string} routeId
   * @return {?mr.dial.Activity}
   */
  getByRouteId(routeId) {
    return this.routeIdToAppInfo_.get(routeId) || null;
  }

  /**
   * Returns the activity corresponding to the given sink ID, or null if there
   * is none.
   * @param {string} sinkId
   * @return {?mr.dial.Activity}
   */
  getBySinkId(sinkId) {
    for (let [routeId, activity] of this.routeIdToAppInfo_) {
      if (activity.route.sinkId == sinkId) {
        return /** @type {!mr.dial.Activity} */ (activity);
      }
    }
    return null;
  }

  /**
   * Removes the activity associated with the given sink ID.
   * @param {string} sinkId
   */
  removeBySinkId(sinkId) {
    const activity = this.getBySinkId(sinkId);
    if (activity) {
      this.routeIdToAppInfo_.delete(activity.route.id);
      this.activityCallbacks_.onActivityRemoved(activity);
    }
  }

  /**
   * Removes the activity associated with the given route ID.
   * @param {string} routeId
   */
  removeByRouteId(routeId) {
    const activity = this.routeIdToAppInfo_.get(routeId);
    if (activity) {
      this.routeIdToAppInfo_.delete(routeId);
      this.activityCallbacks_.onActivityRemoved(activity);
    }
  }

  /**
   * Returns the list of routes associated with the current list of activities.
   * @return {!Array<!mr.Route>}
   */
  getRoutes() {
    return Array.from(
        this.routeIdToAppInfo_.values(), activity => activity.route);
  }

  /**
   * Returns the current list of acitvities.
   * @return {!Array<!mr.dial.Activity>}
   */
  getActivities() {
    return Array.from(this.routeIdToAppInfo_.values());
  }

  /**
   * Returns the number of activities.
   * @return {number}
   */
  getActivityCount() {
    return this.routeIdToAppInfo_.size;
  }

  /**
   * @override
   */
  getStorageKey() {
    return 'dial.ActivityRecords';
  }

  /**
   * @override
   */
  getData() {
    return [Array.from(this.routeIdToAppInfo_)];
  }

  /**
   * @override
   */
  loadSavedData() {
    const savedData = PersistentDataManager.getTemporaryData(this);
    if (savedData) {
      this.routeIdToAppInfo_ = new Map(savedData);
    }
  }
};

exports = ActivityRecords;
