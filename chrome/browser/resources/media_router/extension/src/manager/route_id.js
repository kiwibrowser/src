// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview A routeId includes presentation ID, provider name,
 *  sink ID and media source. See mr.RouteId.getRouteId method for its
 *  format (note the different formatting of routeId for 2UA cases).
 */

goog.provide('mr.RouteId');

goog.require('mr.MediaSourceUtils');

/**
 * @final
 */
mr.RouteId = class {
  constructor() {
    /** @private {string} */
    this.routeId_;

    /** @private {string} */
    this.presentationId_;

    /** @private {string} */
    this.providerName_;

    /** @private {string} */
    this.sinkId_;

    /** @private {string} */
    this.source_;
  }

  /**
   * @param {string} routeId
   * @return {?mr.RouteId} A route ID object if the input string is valid;
   *  null otherwise.
   */
  static create(routeId) {
    if (!routeId.startsWith(mr.RouteId.PREFIX_)) {
      return null;
    }
    const suffix = routeId.substring(mr.RouteId.PREFIX_.length);
    if (!suffix) {
      return null;
    }
    const match = suffix.match(/([^/]*)\/([^-/]*)-([^/]*)\/(.*)/);
    if (!match) {
      return null;
    }
    const obj = new mr.RouteId();
    obj.routeId_ = routeId;
    [, obj.presentationId_, obj.providerName_, obj.sinkId_, obj.source_] =
        match;
    return obj;
  }

  /**
   * @param {string} presentationId
   * @param {string} providerName
   * @param {string} sinkId
   * @param {?string} source
   * @return {string}
   */
  static getRouteId(presentationId, providerName, sinkId, source) {
    // In a 2UA mode, the routeId should be the presentationId.
    if (source && mr.MediaSourceUtils.isPresentationSource(source)) {
      return presentationId;
    }
    return mr.RouteId.PREFIX_ + presentationId + '/' + providerName + '-' +
        sinkId + '/' + source;
  }

  /**
   * @return {string}
   */
  getRouteId() {
    return this.routeId_;
  }

  /**
   * @return {string}
   */
  getPresentationId() {
    return this.presentationId_;
  }

  /**
   * @return {string}
   */
  getProviderName() {
    return this.providerName_;
  }

  /**
   * @return {string}
   */
  getSinkId() {
    return this.sinkId_;
  }

  /**
   * @return {string}
   */
  getSource() {
    return this.source_;
  }
};


/** @private @const {string} */
mr.RouteId.PREFIX_ = 'urn:x-org.chromium:media:route:';
