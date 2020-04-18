// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Provides a way to implement POST requests via XMLHttpRequest.
// Works around https://bugs.webkit.org/show_bug.cgi?id=145410 on WKWebView.
// TODO(crbug.com/740987): Remove |post_request.js| once iOS 10 is dropped.

'use strict';

goog.provide('__crPostRequestWorkaround');

/**
 * Namespace for this file.
 */
__crPostRequestWorkaround = {};

/**
 * Executes a POST request with given parameters and replaces document body with
 * the response.
 * @param {string} url The url of the request.
 * @param {Object<string,string>} headers Request headers to include in POST.
 * Each header value must be expressed as a key-value pair in this object.
 * @param {string} body Request body encoded with Base64.
 * @param {string} contentType Content-Type header value.
 */
__crPostRequestWorkaround.runPostRequest = function(
    url, headers, body, contentType) {

  /**
   * Converts a Base64-encoded string to a blob.
   * @param {string} byteCharacters Base64-encoded data string.
   * @param {string} contentType Corresponding content type.
   * @return {Blob} Binary representation of byteCharacters.
   */
  var base64ToBlob = function(byteCharacters, contentType) {
    contentType = contentType || '';
    var sliceSize = 512;
    var byteArrays = [];
    var byteCount = byteCharacters.length;
    for (var offset = 0; offset < byteCount; offset += sliceSize) {
      var slice = byteCharacters.slice(offset, offset + sliceSize);
      var byteNumbers = new Array(slice.length);
      for (var i = 0; i < slice.length; i++) {
        byteNumbers[i] = slice.charCodeAt(i);
      }
      var byteArray = new Uint8Array(byteNumbers);
      byteArrays.push(byteArray);
    }
    return new Blob(byteArrays, {type: contentType});
  };

  /**
   * Creates and executes a POST request.
   * @param {string} url The url of the request.
   * @param {Object<string,string>} headers Request headers to include in POST.
   * Each header value must be expressed as a key-value pair in this object.
   * @param {string} body Request body encoded with Base64.
   * @param {string} contentType Content-Type header value.
   * @return {number | string}
   */
  var createAndSendPostRequest = function(url, headers, body, contentType) {
    var request = new XMLHttpRequest();
    request.open('POST', url, false);
    for (var key in headers) {
      if (headers.hasOwnProperty(key)) {
        request.setRequestHeader(key, headers[key]);
      }
    }
    var blob = base64ToBlob(atob(body), contentType);
    request.send(blob);
    if (request.status != 200) {
      throw request.status;
    }
    return request.responseText;
  };

  document.open();
  try {
    document.write(/** @type {string} */ (
        createAndSendPostRequest(url, headers, body, contentType)));
    window.webkit.messageHandlers['POSTSuccessHandler'].postMessage('');
  } catch (error) {
    window.webkit.messageHandlers['POSTErrorHandler'].postMessage(error);
  }
  document.close();
};
