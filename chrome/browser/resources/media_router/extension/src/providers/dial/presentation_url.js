// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.module('mr.dial.PresentationUrl');

const Logger = goog.require('mr.Logger');
const base64 = goog.require('mr.base64');


/**
 * Represents a DIAL media source containing information specific to a DIAL
 * launch.
 */
const PresentationUrl = class {
  /**
   * @param {string} appName The DIAL application name.
   * @param {string=} launchParameter DIAL application launch parameter.
   */
  constructor(appName, launchParameter = '') {
    /** @const {string} */
    this.appName = appName;
    /** @const {string} */
    this.launchParameter = launchParameter;
  }

  /**
   * Generates a DIAL Presentation URL using given parameters.
   * @param {string} dialAppName Name of the DIAL app.
   * @param {?string} dialPostData base-64 encoded string of the data for the
   *     DIAL launch.
   * @return {string}
   */
  static getPresentationUrlAsString(dialAppName, dialPostData) {
    const url = new URL('dial:' + dialAppName);
    if (dialPostData) {
      url.searchParams.set('postData', dialPostData);
    }
    return url.toString();
  }

  /**
   * Constructs a DIAL media source from a URL. The URL can take on the new
   * format (with dial: protocol) or the old format (with https: protocol).
   * @param {string} urlString The media source URL.
   * @return {?PresentationUrl} A DIAL media source if the parse was
   *     successful, null otherwise.
   */
  static create(urlString) {
    let url;
    try {
      url = new URL(urlString);
    } catch (err) {
      PresentationUrl.logger_.info('Invalid URL: ' + urlString);
      return null;
    }
    switch (url.protocol) {
      case 'dial:':
        return PresentationUrl.parseDialUrl_(url);
      case 'https:':

        return PresentationUrl.parseLegacyUrl_(url);
      default:
        PresentationUrl.logger_.fine('Unhandled protocol: ' + url.protocol);
        return null;
    }
  }

  /**
   * Parses the given URL using the new DIAL URL format, which takes the form:
   * dial:<App name>?postData=<base64-encoded launch parameters>
   * @param {!URL} url
   * @return {?PresentationUrl}
   * @private
   */
  static parseDialUrl_(url) {
    const appName = url.pathname;
    if (!appName.match(/^\w+$/)) {
      PresentationUrl.logger_.warning('Invalid app name: ' + appName);
      return null;
    }
    let postData = url.searchParams.get('postData') || undefined;
    if (postData) {
      try {
        postData = base64.decodeString(postData);
      } catch (err) {
        PresentationUrl.logger_.warning(
            'Invalid base64 encoded postData:' + postData);
        return null;
      }
    }
    return new PresentationUrl(appName, postData);
  }

  /**
   * Parses the given URL using the legacy format specified in
   * http://goo.gl/8qKAE7
   * Example:
   * http://www.youtube.com/tv#__dialAppName__=YouTube/__dialPostData__=dj0xMjM=
   * @param {!URL} url
   * @return {?PresentationUrl}
   * @private
   */
  static parseLegacyUrl_(url) {
    // Parse URI and get fragment.
    const fragment = url.hash;
    if (!fragment) return null;
    let appName = PresentationUrl.APP_NAME_REGEX_.exec(fragment);
    appName = appName ? appName[1] : null;
    if (!appName) return null;
    appName = decodeURIComponent(appName);

    let postData = PresentationUrl.LAUNCH_PARAM_REGEX_.exec(fragment);
    postData = postData ? postData[1] : undefined;
    if (postData) {
      try {
        postData = base64.decodeString(postData);
      } catch (err) {
        PresentationUrl.logger_.warning(
            'Invalid base64 encoded postData:' + postData);
        return null;
      }
    }
    return new PresentationUrl(appName, postData);
  }
};


/** @const @private {?Logger} */
PresentationUrl.logger_ = Logger.getInstance('mr.dial.PresentationUrl');


/** @const {string} */
PresentationUrl.URN_PREFIX = 'urn:dial-multiscreen-org:dial:application:';


/** @private @const {!RegExp} */
PresentationUrl.APP_NAME_REGEX_ =
    /__dialAppName__=([A-Za-z0-9-._~!$&'()*+,;=%]+)/;


/** @private @const {!RegExp} */
PresentationUrl.LAUNCH_PARAM_REGEX_ = /__dialPostData__=([A-Za-z0-9]+={0,2})/;


exports = PresentationUrl;
