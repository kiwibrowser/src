// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @typedef {{
 *   callback: function(Object, Object):void,
 *   params: Object
 * }}
 */
let TransformPagePointRequest;

/**
 * @typedef {{
 *   type: string,
 *   id: number,
 *   page: number,
 *   x: number,
 *   y: number
 * }}
 */
let TransformPagePointMessage;

(function() {

'use strict';

/**
 * Transforms page to screen coordinates using messages to the plugin.
 */
window.PDFCoordsTransformer = class {
  constructor(postMessageCallback) {
    /** @private {!Map<number,!TransformPagePointRequest>} */
    this.outstandingTransformPagePointRequests_ = new Map();

    /** @private {function(TransformPagePointMessage):void} */
    this.postMessageCallback_ = postMessageCallback;

    /** @private {number} */
    this.nextId_ = 0;
  }

  /**
   * Send a 'transformPagePoint' message to the plugin.
   *
   * @param {function(Object, Object):void} callback Function to call when the
   *     response is received.
   * @param {Object} params User parameters to be used in |callback|.
   * @param {number} page 0-based page number of the page where the point is.
   * @param {number} x x coordinate of the point.
   * @param {number} y y coordinate of the point.
   */
  request(callback, params, page, x, y) {
    this.outstandingTransformPagePointRequests_.set(
        this.nextId_, {callback: callback, params: params});
    this.postMessageCallback_(
        {type: 'transformPagePoint', id: this.nextId_, page: page, x: x, y: y});
    this.nextId_++;
  }

  /**
   * Call when 'transformPagePointReply' is received from the plugin.
   *
   * @param {Object} message The message received from the plugin.
   */
  onReplyReceived(message) {
    const outstandingRequest =
        this.outstandingTransformPagePointRequests_.get(message.data.id);
    this.outstandingTransformPagePointRequests_.delete(message.data.id);
    outstandingRequest.callback(message.data, outstandingRequest.params);
  }
};

}());
