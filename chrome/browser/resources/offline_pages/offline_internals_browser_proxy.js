// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @typedef {{
 *   onlineUrl: string,
 *   creationTime: number,
 *   id: string,
 *   namespace: string,
 *   size: string,
 *   filePath: string,
 *   lastAccessTime: number,
 *   accessCount: number,
 *   isExpired: string,
 *   requestOrigin: string
 * }}
 */
var OfflinePage;

/**
 * @typedef {{
 *   status: string,
 *   onlineUrl: string,
 *   creationTime: number,
 *   id: string,
 *   namespace: string,
 *   lastAttemptTime: number,
 *   requestOrigin: string
 * }}
 */
var SavePageRequest;

/**
 * @typedef {{
 *   modelIsLogging: boolean,
 *   queueIsLogging: boolean,
 *   prefetchIsLogging: boolean
 * }}
 */
var IsLogging;

cr.define('offlineInternals', function() {
  /** @interface */
  function OfflineInternalsBrowserProxy() {}

  OfflineInternalsBrowserProxy.prototype = {
    /**
     * Gets current list of stored pages.
     * @return {!Promise<!Array<OfflinePage>>} A promise firing when the
     *     list is fetched.
     */
    getStoredPages: function() {},

    /**
     * Gets current offline queue requests.
     * @return {!Promise<!Array<SavePageRequest>>} A promise firing when the
     *     request queue is fetched.
     */
    getRequestQueue: function() {},

    /**
     * Deletes a set of pages from stored pages
     * @param {!Array<string>} ids A list of page IDs to delete.
     * @return {!Promise<!string>} A promise firing when the selected
     *     pages are deleted.
     */
    deleteSelectedPages: function(ids) {},

    /**
     * Deletes a set of requests from the request queue
     * @param {!Array<string>} ids A list of request IDs to delete.
     * @return {!Promise<!string>} A promise firing when the selected
     *     pages are deleted.
     */
    deleteSelectedRequests: function(ids) {},

    /**
     * Sets whether to record logs for stored pages.
     * @param {boolean} shouldLog True if logging should be enabled.
     */
    setRecordPageModel: function(shouldLog) {},

    /**
     * Sets whether to record logs for scheduled requests.
     * @param {boolean} shouldLog True if logging should be enabled.
     */
    setRecordRequestQueue: function(shouldLog) {},

    /**
     * Sets whether to record logs for prefetching.
     * @param {boolean} shouldLog True if logging should be enabled.
     */
    setRecordPrefetchService: function(shouldLog) {},

    /**
     * Gets the currently recorded logs.
     * @return {!Promise<!Array<string>>} A promise firing when the
     *     logs are retrieved.
     */
    getEventLogs: function() {},

    /**
     * Gets the state of logging (on/off).
     * @return {!Promise<!IsLogging>} A promise firing when the state
     *     is retrieved.
     */
    getLoggingState: function() {},

    /**
     * Adds the given url to the background loader queue.
     * @param {string} url Url of the page to load later.
     * @return {!Promise<boolean>} A promise firing after added to queue.
     *     Promise will return true if url has been successfully added.
     */
    addToRequestQueue: function(url) {},

    /**
     * Gets the current network status in string form.
     * @return {!Promise<string>} A promise firing when the network status
     *     is retrieved.
     */
    getNetworkStatus: function() {},

    /**
     * Schedules the default NWake task.  The returned Promise will reject if
     *     there is an error while scheduling.
     * @return {!Promise<string>} A promise firing when the task has been
     *     scheduled.
     */
    scheduleNwake: function() {},

    /**
     * Cancels NWake task.
     * @return {!Promise} A promise firing when the task has been cancelled. The
     *     returned Promise will reject if there is an error.
     */
    cancelNwake: function() {},

    /**
     * Shows the prefetching notification with an example origin.
     * @return {!Promise<string>} A promise firing when the notification has
     *   been shown.
     */
    showPrefetchNotification: function() {},

    /**
     * Sends and processes a request to generate page bundle.
     * @param {string} urls A list of comma-separated URLs.
     * @return {!Promise<string>} A string describing the result.
     */
    generatePageBundle: function(urls) {},

    /**
     * Sends and processes a request to get operation.
     * @param {string} name Name of operation.
     * @return {!Promise<string>} A string describing the result.
     */
    getOperation: function(name) {},

    /**
     * Downloads an archive.
     * @param {string} name Name of archive to download.
     */
    downloadArchive: function(name) {},
  };

  /**
   * @constructor
   * @implements {offlineInternals.OfflineInternalsBrowserProxy}
   */
  function OfflineInternalsBrowserProxyImpl() {}
  cr.addSingletonGetter(OfflineInternalsBrowserProxyImpl);

  OfflineInternalsBrowserProxyImpl.prototype = {
    /** @override */
    getStoredPages: function() {
      return cr.sendWithPromise('getStoredPages');
    },

    /** @override */
    getRequestQueue: function() {
      return cr.sendWithPromise('getRequestQueue');
    },

    /** @override */
    deleteSelectedPages: function(ids) {
      return cr.sendWithPromise('deleteSelectedPages', ids);
    },

    /** @override */
    deleteSelectedRequests: function(ids) {
      return cr.sendWithPromise('deleteSelectedRequests', ids);
    },

    /** @override */
    setRecordPageModel: function(shouldLog) {
      chrome.send('setRecordPageModel', [shouldLog]);
    },

    /** @override */
    setRecordRequestQueue: function(shouldLog) {
      chrome.send('setRecordRequestQueue', [shouldLog]);
    },

    /** @override */
    setRecordPrefetchService: function(shouldLog) {
      chrome.send('setRecordPrefetchService', [shouldLog]);
    },

    /** @override */
    getEventLogs: function() {
      return cr.sendWithPromise('getEventLogs');
    },

    /** @override */
    getLoggingState: function() {
      return cr.sendWithPromise('getLoggingState');
    },

    /** @override */
    addToRequestQueue: function(url) {
      return cr.sendWithPromise('addToRequestQueue', url);
    },

    /** @override */
    getNetworkStatus: function() {
      return cr.sendWithPromise('getNetworkStatus');
    },

    /** @override */
    scheduleNwake: function() {
      return cr.sendWithPromise('scheduleNwake');
    },

    /** @override */
    cancelNwake: function() {
      return cr.sendWithPromise('cancelNwake');
    },

    /** @override */
    showPrefetchNotification: function() {
      return cr.sendWithPromise('showPrefetchNotification');
    },

    /** @override */
    generatePageBundle: function(urls) {
      return cr.sendWithPromise('generatePageBundle', urls);
    },

    /** @override */
    getOperation: function(name) {
      return cr.sendWithPromise('getOperation', name);
    },

    /** @override */
    downloadArchive: function(name) {
      chrome.send('downloadArchive', [name]);
    },
  };

  return {
    OfflineInternalsBrowserProxy: OfflineInternalsBrowserProxy,
    OfflineInternalsBrowserProxyImpl: OfflineInternalsBrowserProxyImpl
  };
});
