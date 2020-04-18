// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Interface to a mirroring session.

 */

goog.provide('mr.mirror.Session');

goog.require('mr.TabUtils');
goog.require('mr.mirror.Activity');


/**
 * Creates a new MirrorSession.  Do not call, as this is an abstract base class.
 */
mr.mirror.Session = class {
  /**
   * @param {!mr.Route} route
   * @param {?function(!mr.Route, !mr.mirror.Activity)=} onActivityUpdate
   */
  constructor(route, onActivityUpdate = null) {
    /**
     * The media route associated with this mirror session.
     * @protected @const {!mr.Route}
     */
    this.route = route;

    /**
     * Called when this.activity_ has changed.
     * @private {?function(!mr.Route, !mr.mirror.Activity)}
     */
    this.onActivityUpdate_ = onActivityUpdate;

    /**
     * The activity description for keeping UI strings up to date.
     * @protected {!mr.mirror.Activity}
     */
    this.activity = mr.mirror.Activity.createFromRoute(route);

    /** @type {?number} */
    this.tabId = null;

    /** @type {?Tab} */
    this.tab = null;

    /** @type {boolean} */
    this.isRemoting = false;
  }

  /**
   * Sets the callback for handling activity updates.
   * @param {!function(!mr.Route, !mr.mirror.Activity)} onActivityUpdate
   */
  setOnActivityUpdate(onActivityUpdate) {
    this.onActivityUpdate_ = onActivityUpdate;
  }

  /**
   * @param {number} tabId The id of the tab being captured, if any.
   * @return {!Promise<void>}
   */
  setTabId(tabId) {
    if (this.tabId != tabId) {
      this.tabId = tabId;
      return mr.TabUtils.getTab(tabId).then((tab) => {
        this.tab = tab;
        this.onActivityUpdated();
      });
    } else {
      return Promise.resolve();
    }
  }

  /**
   * Handles tab updated event.
   *
   * @param {number} tabId the ID of the tab.
   * @param {!TabChangeInfo} changeInfo The changes to the state of the tab.
   * @param {!Tab} tab The tab.
   */
  onTabUpdated(tabId, changeInfo, tab) {
    if (tabId != this.tabId) return;
    if (changeInfo.status == 'complete' ||
        (!!changeInfo.favIconUrl && tab.status == 'complete')) {
      this.tab = tab;
      this.onActivityUpdated();
    }
  }

  /**
   * Called when the activity object for the mirror session needs to be updated.
   * This happens when e.g. the tab being mirrored navigates, changes title, or
   * switches in or out of media remoting.
   */
  onActivityUpdated() {
    if (this.tab) {
      this.activity.setContentTitle(this.tab.title);
      this.activity.setIncognito(this.tab.incognito);
      const url = new URL(this.tab.url);
      if (url.protocol == 'file:') {
        this.activity.setType(mr.mirror.Activity.Type.MIRROR_FILE);
        this.activity.setOrigin(null);
      } else {
        this.activity.setType(
            this.isRemoting ? mr.mirror.Activity.Type.MEDIA_REMOTING :
                              mr.mirror.Activity.Type.MIRROR_TAB);
        if (url.protocol == 'https:') {
          // OK to drop the protocol for secure origins.
          this.activity.setOrigin(url.origin.substr(8));
        } else {
          this.activity.setOrigin(url.origin);
        }
      }
    }
    this.route.description = this.activity.getRouteDescription();
    this.onActivityUpdate_ && this.onActivityUpdate_(this.route, this.activity);
    this.sendActivityToSink();
  }

  /**
   * @return {!mr.Route}
   */
  getRoute() {
    return this.route;
  }

  /**
   * @return {!mr.mirror.Activity}
   */
  getActivity() {
    return this.activity;
  }

  /**
   * Starts the mirroring session. The |mediaStream| must provide one audio
   * and/or one video track. It is illegal to call start() more than once on the
   * same session, even after stop() has been called. See updateStream(), or
   * else create a new instance to re-start a session.
   * @param {!MediaStream} mediaStream The media stream that has the audio
   *    and/or video track.
   * @return {!Promise<mr.mirror.Session>} Fulfilled when the
   *     session has been created and transports have started for the streams.
   */
  start(mediaStream) {}

  /**
   * @return {boolean} Whether the session supports updating its media stream
   *     while it is active.
   */
  supportsUpdateStream() {
    return false;
  }

  /**
   * Updates the media stream that the session is using.
   * @param {!MediaStream} mediaStream
   * @return {!Promise}
   */
  updateStream(mediaStream) {}

  /**
   * Stops the mirroring session. The underlying streams and transports are
   * stopped and destroyed. Sessions cannot be re-started. Instead, either use
   * updateStream() or create a new instance to re-start a session.
   * @return {!Promise<void>} Fulfilled with once the session has been stopped.
   *     The promise is rejected on error.
   */
  stop() {
    return Promise.resolve();
  }

  /**
   * Sends updated activity info directly to the sink.
   */
  sendActivityToSink() {}
};
