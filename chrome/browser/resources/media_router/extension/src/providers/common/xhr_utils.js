// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Utilities for dealing with XmlHttpRequests.
 */

goog.provide('mr.XhrUtils');

goog.require('mr.Logger');


/**
 * Logs the outcome of a XmlHttpRequest.
 * @param {mr.Logger} logger Where to log.
 * @param {string} action The operation that created the Fetch.
 * @param {string} method The HTTP method.
 * @param {!XMLHttpRequest} xhr The response from Fetch.
 */
mr.XhrUtils.logRawXhr = function(logger, action, method, xhr) {
  const logString = mr.XhrUtils.getStatusString_(
      action, method, xhr.responseURL, xhr.status, xhr.statusText);
  if (mr.XhrUtils.isSuccess(xhr)) {
    logger.info(logString);
  } else {
    logger.fine(logString);
  }
};


/**
 * Returns true if the given XMLHttpRequest represents a successful response.
 * @param {!XMLHttpRequest} xhr
 * @return {boolean}
 */
mr.XhrUtils.isSuccess = function(xhr) {
  return xhr.status >= 200 && xhr.status <= 299;
};


/**
 * Returns a loggable string with the given parameters.
 * @param {string} action
 * @param {string} method
 * @param {string} url
 * @param {number} status
 * @param {string} statusText
 * @return {string}
 * @private
 */
mr.XhrUtils.getStatusString_ = function(
    action, method, url, status, statusText) {
  return `[${action}]: ${method} ${url} => ${status} (${statusText})`;
};


/**
 * Parses xmlText into an XML document.
 * @param {string} xmlText A serialized XML document.
 * @return {?Document} An XML document if the parse was successful, null
 *     otherwise.
 */
mr.XhrUtils.parseXml = function(xmlText) {
  const xml = new DOMParser().parseFromString(xmlText, 'text/xml');
  // A failed parse returns an HTML document with a <parsererror> element.
  return xml.getElementsByTagNameNS(
                'http://www.w3.org/1999/xhtml', 'parsererror')
             .length ?
      null :
      xml;
};
