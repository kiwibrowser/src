// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Client wrapper for interacting with DIAL devices and data
 * structures for responses.
 */

goog.module('mr.dial.Client');
goog.module.declareLegacyNamespace();

const Assertions = goog.require('mr.Assertions');
const Logger = goog.require('mr.Logger');
const NetUtils = goog.require('mr.NetUtils');
const XhrManager = goog.require('mr.XhrManager');
const XhrUtils = goog.require('mr.XhrUtils');

/**
 * Possible states of a DIAL application.
 * @enum {string}
 */
const DialAppState = {
  /** The app is running. */
  RUNNING: 'running',
  /** The app is not running.  */
  STOPPED: 'stopped',
  /** The app can be installed. */
  INSTALLABLE: 'installable',
  /**
   * An error was encountered getting the state.

   */
  ERROR: 'error'
};


/**
 * Holds data parsed from a DIAL GET response.
 */
const AppInfo = class {
  constructor() {
    /**
     * The application name.  Mandatory.
     * @type {string}
     */
    this.name = 'unknown';

    /**
     * The reported state of the application.
     * @type {DialAppState}
     */
    this.state = DialAppState.ERROR;

    /**
     * If the application's state is INSTALLABLE, then the URL where the app
     * can be installed.
     * @type {?string}
     */
    this.installUrl = null;

    /**
     * Whether the DELETE operation is supported.
     * @type {boolean}
     */
    this.allowStop = true;

    /**
     * If the applications's state is RUNNING, a resource identifier for the
     * running application.
     * @type {?string}
     */
    this.resource = null;

    /**
     * Application-specific data included with the GET response that is not part
     * of the official specifciations.
     * @type {!Object<string, string>}
     */
    this.extraData = {};
  }
};


/**
 * Indicates that the DIAL sink returned NOT_FOUND in response to a GET request.
 */
const AppInfoNotFoundError = class extends Error {
  constructor() {
    super();
  }
};


/**
 * Wrapper for a DIAL sink used for communicating with it.
 */
const Client = class {
  /**
   * @param {!mr.dial.Sink} sink
   * @param {!XhrManager=} xhrManager Manager for all requests.
   */
  constructor(sink, xhrManager = Client.getXhrManager_()) {
    // NOTE(mfoltz,haibinlu): We do not assert if the sink supports DIAL,
    // since combined discovery uses DialClient to check app info to see if a
    // mDNS-discovered sink is also a DIAL sink.
    Assertions.assert(
        sink.getDialAppUrl(), 'Receiver must have a DIAL app URL set.');

    /** @private @const {!mr.dial.Sink} */
    this.sink_ = sink;

    /** @private @const {!XhrManager} */
    this.xhrManager_ = xhrManager;

    /** @private @const {?Logger} */
    this.logger_ = Logger.getInstance('mr.dial.Client');
  }

  /**
   * @param {!mr.dial.Sink} sink
   * @return {!Client}
   */
  static create(sink) {
    return new Client(sink);
  }

  /**
   * Returns the default XhrManager, creating it if necessary.
   * @return {!XhrManager}
   * @private
   */
  static getXhrManager_() {
    if (!Client.xhrManager_) {
      Client.xhrManager_ = new XhrManager(
          /* maxRequests */ 10,
          /* defaultTimeoutMillis */ 2000,
          /* defaultNumAttempts */ 1);
    }
    return Client.xhrManager_;
  }

  /**
   * @param {string} state A string representing a DIAL application state.
   * @return {DialAppState} The corresponding state or ERROR if the
   *     state is invalid.
   * @private
   */
  static parseDialAppState_(state) {
    switch (state) {
      case 'running':
        return DialAppState.RUNNING;
      case 'stopped':
        return DialAppState.STOPPED;
      default:
        return DialAppState.ERROR;
    }
  }

  /**
   * Launches an application on the sink.
   * @param {string} appName Name of the DIAL application to launch.
   * @param {string} postData Data to include in the HTTP POST request.
   * @return {!Promise<void>} Fulfilled when the operation completes
   *     successfully. Rejected otherwise.
   */
  launchApp(appName, postData) {
    return this.xhrManager_
        .send(
            this.getAppUrl_(appName), 'POST', postData, {timeoutMillis: 15000})
        .then(xhr => this.handleResponse_('launchApp', 'POST', xhr));
  }

  /**
   * Stops a running application on the sink.
   * @param {string} appName Name of the DIAL application to stop.
   * @return {!Promise<void>} Fulfilled when the operation completes
   *     successfully. Rejected otherwise.
   */
  stopApp(appName) {
    return this.xhrManager_.send(this.getAppUrl_(appName), 'DELETE')
        .then(xhr => this.handleResponse_('stopApp', 'DELETE', xhr));
  }

  /**
   * Gets information about a running application on the sink.
   * @param {string} appName Name of the DIAL application to get info from.
   * @return {!Promise<!AppInfo>} Fulfilled with AppInfo. Rejected if the
   *     operation did not complete successfully. In the case of the sink
   *     returning NOT_FOUND for the request, AppInfoNotFoundError will be
   *     thrown.
   */
  getAppInfo(appName) {
    return this.xhrManager_
        .send(this.getAppUrl_(appName), 'GET', undefined, {numAttempts: 3})
        .then(xhr => this.handleGetAppInfoResponse_(appName, xhr));
  }

  /**
   * Parses the response from a Xhr GET request.
   * @param {string} appName App nam used in the request.
   * @param {!XMLHttpRequest} xhr
   * @return {!AppInfo}
   * @private
   */
  handleGetAppInfoResponse_(appName, xhr) {
    XhrUtils.logRawXhr(this.logger_, 'GetAppInfo', 'GET', xhr);
    if (!XhrUtils.isSuccess(xhr)) {
      if (xhr.status == NetUtils.HttpStatus.NOT_FOUND) {
        throw new AppInfoNotFoundError();
      } else {
        throw new Error(`Response error: ${xhr.status}`);
      }
    }

    const xml = XhrUtils.parseXml(xhr.responseText);
    if (!xml) {
      this.logger_.info('Invalid or empty response');
      throw new Error('Invalid or empty response');
    }

    const service = xml.getElementsByTagName('service');
    if (!service || service.length != 1) {
      this.logger_.info('Invalid GET response (invalid service)');
      throw new Error('Invalid GET response (invalid service)');
    }
    const appInfo = new AppInfo();
    for (var i = 0, l = service[0].childNodes.length; i < l; i++) {
      const node = service[0].childNodes[i];
      if (node.nodeName == 'state') {
        appInfo.state = Client.parseDialAppState_(node.textContent);
      } else if (node.nodeName == 'name') {
        appInfo.name = node.textContent;
      } else if (node.nodeName == 'link') {
        appInfo.resource = node.getAttribute('href');
      } else if (node.nodeName == 'options') {
        // The default value for allowStop is true per DIAL spec.
        appInfo.allowStop = (node.getAttribute('allowStop') != 'false');
      } else {
        appInfo.extraData[node.nodeName] = node.innerHTML;
      }
    }

    // Validate mandatory fields (name, state).
    if (appInfo.name == 'unknown') {
      this.logger_.info('GET response missing name value');
      throw new Error('GET response missing name value');
    }

    if (appInfo.name != appName) {
      this.logger_.info('GET app name mismatch');
      throw new Error('GET app name mismatch');
    }

    if (appInfo.state == DialAppState.ERROR) {
      this.logger_.info('GET response missing state value');
      throw new Error('GET response missing state value');
    }

    // Parse state.
    const installable = /installable=(.+)/.exec(appInfo.state);
    if (installable && installable[1]) {
      appInfo.state = DialAppState.INSTALLABLE;
      appInfo.installUrl = installable[1];
    } else if (
        appInfo.state == DialAppState.RUNNING ||
        appInfo.state == DialAppState.STOPPED) {
      // Valid state.  Continue.
    } else {
      this.logger_.info('GET response has invalid state value');
      throw new Error('GET response has invalid state value');
    }

    // Success!
    return appInfo;
  }

  /**
   * Returns the URL used to communicate with a given DIAL application.
   * @param {string} appName The name of the DIAL application.
   * @return {string} The URL for the activity.
   * @private
   */
  getAppUrl_(appName) {
    let appUrl = this.sink_.getDialAppUrl();
    if (appUrl.charAt(appUrl.length - 1) != '/') {
      appUrl += '/';
    }
    return appUrl + appName;
  }

  /**
   * Logs the given response and returns a Promise that resolves if it indicates
   * success.
   * @param {string} action Name of the operation that created the request.
   * @param {string} method The HTTP method.
   * @param {!XMLHttpRequest} xhr
   * @return {!Promise<void>} Resolves if the response indicates success,
   *     rejected otherwise.
   * @private
   */
  handleResponse_(action, method, xhr) {
    return new Promise((resolve, reject) => {
      XhrUtils.logRawXhr(this.logger_, action, method, xhr);
      if (XhrUtils.isSuccess(xhr)) {
        resolve();
      } else {
        reject(Error(xhr.statusText));
      }
    });
  }
};


/**
 * Lazily instantiated and shared between DialClient instances.
 * @private {?XhrManager}
 */
Client.xhrManager_ = null;


exports.AppInfo = AppInfo;
exports.AppInfoNotFoundError = AppInfoNotFoundError;
exports.Client = Client;
exports.DialAppState = DialAppState;
