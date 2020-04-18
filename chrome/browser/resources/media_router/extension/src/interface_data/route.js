// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview The media route data object shared between component extension
 *  and Chrome media router.
 */

goog.provide('mr.Route');
goog.require('mr.RouteId');

mr.Route = class {
  /**
   * @param {string} id The route ID.
   * @param {string} presentationId The ID of the associated presentation.
   * @param {string} sinkId The ID of the sink running this route.
   * @param {?string} sourceUrn The media source being sent through this route.
   * @param {boolean} isLocal Whether the route is requested locally.
   * @param {string} description
   * @param {?string} iconUrl
   */
  constructor(
      id, presentationId, sinkId, sourceUrn, isLocal, description, iconUrl) {
    /**
     * @type {string}
     * @export
     */
    this.id = id;

    /**
     * The ID of the presentation associated with this route.
     * @type {string}
     * @export
     */
    this.presentationId = presentationId;

    /**
     * The ID of the sink associated with this route.
     * @type {string}
     * @export
     */
    this.sinkId = sinkId;

    /**
     * The media source being sent through this route.
     * Non-null if |isLocal| is true.
     * For discovered routes, the media source may not be available.
     * @type {?string}
     * @export
     */
    this.mediaSource = sourceUrn;

    /**
     * Whether the route was created locally or is associated with a Cast
     * session
     * that was created locally.
     * @type {boolean}
     * @export
     */
    this.isLocal = isLocal;

    /**
     * @type {string}
     * @export
     */
    this.description = description;

    /**
     * @type {?string}
     * @export
     */
    this.iconUrl = iconUrl;

    /**
     * When false, this route cannot be stopped by the provider managing it.
     * @type {boolean}
     * @export
     */
    this.allowStop = true;

    /**

     * @type {?string}
     * @export
     */
    this.customControllerPath = null;

    /**

     * @type {boolean}
     * @export
     */
    this.supportsMediaRouteController = false;

    /**
     * The type of controller associated with this route, or kNone if controller
     * is not supported for the route.

     * @type {mojo.RouteControllerType}
     * @export
     */
    this.controllerType =
        mojo && mojo.RouteControllerType && mojo.RouteControllerType.kNone;

    /**
     * If set to true, this route should be displayed for |sinkId| in UI.
     * @type {boolean}
     * @export
     */
    this.forDisplay = true;

    /**
     * If true, the route was created by an off the record (incognito) browser
     * profile.
     * @type {boolean}
     * @export
     */
    this.offTheRecord = false;

    /**
     * This field is used to identify routes that were created locally as
     * opposed
     * to isLocal which can identify both locally created routes and non-local
     * routes associated with a Cast session that was created locally. The
     * initial
     * value of this field is the same as isLocal.
     * @type {boolean}
     */
    this.createdLocally = isLocal;

    /**
     * If true, the route was created for offscreen presentation (1-UA mode).
     * @type {boolean}
     * @export
     */
    this.isOffscreenPresentation = false;
  }

  /**
   * @param {!string} presentationId
   * @param {!string} providerName
   * @param {!string} sinkId The ID of the sink running this route.
   * @param {?string} source The media source being sent through this route.
   * @param {!boolean} isLocal Whether the route is requested locally.
   * @param {!string} description
   * @param {?string} iconUrl
   * @return {!mr.Route}
   */
  static createRoute(
      presentationId, providerName, sinkId, source, isLocal, description,
      iconUrl) {
    return new mr.Route(
        mr.RouteId.getRouteId(presentationId, providerName, sinkId, source),
        presentationId, sinkId, source, isLocal, description, iconUrl);
  }
};

/**
 * @typedef {{
 *   tabId: (?number|undefined),
 *   sessionId: string,
 *   sinkIpAddress: string,
 *   sinkModelName: string,
 *   sinkFriendlyName: (string|undefined),
 *   activity: (!mr.mirror.Activity|undefined)
 * }}
 */
mr.Route.MirrorInitData;


/**
 * The field is only used inside component extension to pass MRP specific data
 * to the corresponding mirroring service. For example, cast streaming needs
 * sink IP address and session ID.
 * @type {mr.Route.MirrorInitData}
 */
mr.Route.prototype.mirrorInitData;
