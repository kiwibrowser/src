// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Contains the possible states a ServiceEntry can be in.
 * @enum {string}
 */
var ServiceEntryState = {
  NEW: 'NEW',
  AVAILABLE: 'AVAILABLE',
  ACTIVE: 'ACTIVE',
  PAUSED: 'PAUSED',
  COMPLETE: 'COMPLETE',
};

/**
 * Contains the possible states a ServiceEntry's driver can be in.
 * @enum {string}
 */
var DriverEntryState = {
  IN_PROGRESS: 'IN_PROGRESS',
  COMPLETE: 'COMPLETE',
  CANCELLED: 'CANCELLED',
  INTERRUPTED: 'INTERRUPTED',
};

/**
 * Contains the possible results a ServiceEntry can have.
 * @enum {string}
 */
var ServiceEntryResult = {
  SUCCEED: 'SUCCEED',
  FAIL: 'FAIL',
  ABORT: 'ABORT',
  TIMEOUT: 'TIMEOUT',
  UNKNOWN: 'UNKNOWN',
  CANCEL: 'CANCEL',
  OUT_OF_RETRIES: 'OUT_OF_RETRIES',
  OUT_OF_RESUMPTIONS: 'OUT_OF_RESUMPTIONS',
};

/**
 * Contains the possible results of a ServiceRequest.
 * @enum {string}
 */
var ServiceRequestResult = {
  ACCEPTED: 'ACCEPTED',
  BACKOFF: 'BACKOFF',
  UNEXPECTED_CLIENT: 'UNEXPECTED_CLIENT',
  UNEXPECTED_GUID: 'UNEXPECTED_GUID',
  CLIENT_CANCELLED: 'CLIENT_CANCELLED',
  INTERNAL_ERROR: 'INTERNAL_ERROR',
};

/**
 * @typedef {{
 *   serviceState: string,
 *   modelStatus: string,
 *   driverStatus: string,
 *   fileMonitorStatus: string
 * }}
 */
var ServiceStatus;

/**
 * @typedef {{
 *   client: string,
 *   guid: string,
 *   state: !ServiceEntryState,
 *   url: string,
 *   bytes_downloaded: number,
 *   result: (!ServiceEntryResult|undefined),
 *   driver: {
 *     state: !DriverEntryState,
 *     paused: boolean,
 *     done: boolean
 *   }
 * }}
 */
var ServiceEntry;

/**
 * @typedef {{
 *   client: string,
 *   guid: string,
 *   result: !ServiceRequestResult
 * }}
 */
var ServiceRequest;

cr.define('downloadInternals', function() {
  /** @interface */
  class DownloadInternalsBrowserProxy {
    /**
     * Gets the current status of the Download Service.
     * @return {!Promise<ServiceStatus>} A promise firing when the service
     *     status is fetched.
     */
    getServiceStatus() {}

    /**
     * Gets the current list of downloads the Download Service is aware of.
     * @return {!Promise<!Array<!ServiceEntry>>} A promise firing when the list
     *     of downloads is fetched.
     */
    getServiceDownloads() {}

    /**
     * Starts a download with the Download Service.
     * @param {string} url The download URL.
     */
    startDownload(url) {}
  }

  /**
   * @implements {downloadInternals.DownloadInternalsBrowserProxy}
   */
  class DownloadInternalsBrowserProxyImpl {
    /** @override */
    getServiceStatus() {
      return cr.sendWithPromise('getServiceStatus');
    }

    /** @override */
    getServiceDownloads() {
      return cr.sendWithPromise('getServiceDownloads');
    }

    /** @override */
    startDownload(url) {
      return cr.sendWithPromise('startDownload', url);
    }
  }

  cr.addSingletonGetter(DownloadInternalsBrowserProxyImpl);

  return {
    DownloadInternalsBrowserProxy: DownloadInternalsBrowserProxy,
    DownloadInternalsBrowserProxyImpl: DownloadInternalsBrowserProxyImpl
  };
});
