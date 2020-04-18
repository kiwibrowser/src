// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Mirroring activities for display in the UI.
 */

goog.module('mr.mirror.Activity');
goog.module.declareLegacyNamespace();

const Assertions = goog.require('mr.Assertions');
const MediaSourceUtils = goog.require('mr.MediaSourceUtils');


/**
 * Possible mirroring activities.
 * @enum {string}
 */
const Type = {
  MIRROR_TAB: 'mirror_tab',
  MIRROR_DESKTOP: 'mirror_desktop',
  MIRROR_FILE: 'mirror_file',
  MEDIA_REMOTING: 'media_remoting',
  PRESENTATION: 'presentation'
};


/**
 * Describes a mirroring activity for display in the UI.
 */
const Activity = class {
  /**
   * Constructs a new Activity describing a mirroring route.  origin is
   * required, unless the activityType is MIRROR_DESKTOP.  Both contentTitle and
   * origin may change over the course of a mirroring activity from tab
   * navigation.
   *
   * @param {!Type} type
   * @param {boolean} incognito
   * @param {?string=} origin The top level origin of the tab being cast.
   */
  constructor(type, incognito, origin = null) {
    /** @private {!Type} */
    this.type_ = type;

    /** @private {boolean} */
    this.incognito_ = incognito;

    /** @private {?string} */
    this.origin_ = origin;

    /** @private {?string} */
    this.contentTitle_ = null;
  }

  /**
   * @param {!mr.Route} route A mirroring route
   * @return {!Activity} Intital activity object for route
   */
  static createFromRoute(route) {
    let type;
    let origin = null;
    const source = /** @type {string} */ (Assertions.assert(route.mediaSource));
    if (MediaSourceUtils.isPresentationSource(source)) {
      type = Type.PRESENTATION;
      origin = new URL(source).origin;
    } else if (MediaSourceUtils.isTabMirrorSource(source)) {
      // Tab mirroring routes may switch to MEDIA_REMOTING or MIRROR_FILE after
      // they have begun.
      type = Type.MIRROR_TAB;
    } else if (MediaSourceUtils.isDesktopMirrorSource(source)) {
      type = Type.MIRROR_DESKTOP;
    }
    Assertions.assert(type, `Unexpected mediaSource ${source}`);
    return new Activity(
        /** @type {Type} */ (type), route.offTheRecord, origin);
  }

  /** @param {Type} type Sets the activity type. */
  setType(type) {
    this.type_ = type;
  }

  /** @param {?string} origin Sets the current origin of the activity. */
  setOrigin(origin) {
    this.origin_ = origin;
  }

  /** @param {?string} contentTitle Sets the content title of the activity. */
  setContentTitle(contentTitle) {
    this.contentTitle_ = contentTitle;
  }

  /** @param {boolean} incognito Sets the incognito status of the activity. */
  setIncognito(incognito) {
    this.incognito_ = incognito;
  }

  /** @return {string} The string to use for the route's description. */
  getRouteDescription() {
    switch (this.type_) {
      case Type.MIRROR_TAB:
        return this.origin_ ? `Casting tab (${this.origin_})` : 'Casting tab';
      case Type.MIRROR_DESKTOP:
        return 'Casting desktop';
      case Type.MIRROR_FILE:
        return 'Casting local content';
      case Type.MEDIA_REMOTING:
        return this.origin_ ? `Casting media (${this.origin_})` :
                              `Casting media`;
      case Type.PRESENTATION:
        return `Casting ${this.origin_ || 'site'}`;
      default:
        Assertions.assert(false, 'Unexpected type ' + this.type_);
        return '';
    }
  }

  /** @return {string} The string to use for the route's media status. */
  getRouteMediaStatus() {
    if (this.type_ == Type.MIRROR_DESKTOP) {
      return '';
    }
    return this.contentTitle_ || '';
  }

  /** @return {string} The string to broadcast to other Cast senders. */
  getCastRemoteTitle() {
    if (this.incognito_) return 'Casting active';
    switch (this.type_) {
      case Type.MIRROR_TAB:
        return 'Casting tab';
      case Type.MIRROR_DESKTOP:
        return 'Casting desktop';
      case Type.MIRROR_FILE:
        return 'Casting local content';
      case Type.MEDIA_REMOTING:
        return 'Casting media';
      case Type.PRESENTATION:
        return 'Casting site';
      default:
        Assertions.assert(false, 'Unexpected type ' + this.type_);
        return '';
    }
  }
};

exports = Activity;
exports.Type = Type;
