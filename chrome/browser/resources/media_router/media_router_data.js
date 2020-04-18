// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Any strings used here will already be localized. Values such as
// CastMode.type or IDs will be defined elsewhere and determined later.

cr.exportPath('media_router');

/**
 * This corresponds to the C++ MediaCastMode, with the exception of AUTO.
 * See below for details. Note to support fast bitset operations, the values
 * here are (1 << [corresponding value in MR]).
 * @enum {number}
 */
media_router.CastModeType = {
  // Note: AUTO mode is only used to configure the sink list container to show
  // all sinks. Individual sinks are configured with a specific cast mode
  // (PRESENTATION, TAB_MIRROR, DESKTOP_MIRROR).
  AUTO: -1,
  PRESENTATION: 0x1,
  TAB_MIRROR: 0x2,
  DESKTOP_MIRROR: 0x4,
  LOCAL_FILE: 0x8,
};

/**
 * The ESC key maps to KeyboardEvent.key value 'Escape'.
 * @const {string}
 */
media_router.KEY_ESC = 'Escape';

/**
 * This corresponds to the C++ MediaRouterMetrics
 * MediaRouterRouteCreationOutcome.
 * @enum {number}
 */
media_router.MediaRouterRouteCreationOutcome = {
  SUCCESS: 0,
  FAILURE_NO_ROUTE: 1,
  FAILURE_INVALID_SINK: 2,
};

/**
 * This corresponds to the C++ MediaRouterMetrics MediaRouterUserAction.
 * @enum {number}
 */
media_router.MediaRouterUserAction = {
  CHANGE_MODE: 0,
  START_LOCAL: 1,
  STOP_LOCAL: 2,
  CLOSE: 3,
  STATUS_REMOTE: 4,
  REPLACE_LOCAL_ROUTE: 5,
};

/**
 * The possible states of the Media Router dialog. Used to determine which
 * components to show.
 * @enum {string}
 */
media_router.MediaRouterView = {
  CAST_MODE_LIST: 'cast-mode-list',
  FILTER: 'filter',
  ISSUE: 'issue',
  ROUTE_DETAILS: 'route-details',
  SINK_LIST: 'sink-list',
};

/**
 * The minimum number of sinks to have to enable the search input strictly for
 * filtering (i.e. the Media Router doesn't support search so the search input
 * only filters existing sinks).
 * @const {number}
 */
media_router.MINIMUM_SINKS_FOR_SEARCH = 20;

/**
 * The states that media can be in.
 * @enum {number}
 */
media_router.PlayState = {
  PLAYING: 0,
  PAUSED: 1,
  BUFFERING: 2,
};

/**
 * This corresponds to the C++ MediaSink IconType, and the order must stay in
 * sync.
 * @enum {number}
 */
media_router.SinkIconType = {
  CAST: 0,
  CAST_AUDIO_GROUP: 1,
  CAST_AUDIO: 2,
  MEETING: 3,
  HANGOUT: 4,
  EDUCATION: 5,
  WIRED_DISPLAY: 6,
  GENERIC: 7,
};

/**
 * @enum {string}
 */
media_router.SinkStatus = {
  IDLE: 'idle',
  ACTIVE: 'active',
  REQUEST_PENDING: 'request_pending'
};

cr.define('media_router', function() {
  'use strict';

  /**
   * @param {number} type The type of cast mode.
   * @param {string} description The description of the cast mode.
   * @param {?string} host The hostname of the site to cast.
   * @param {boolean} isForced True if the mode is forced.
   * @constructor
   * @struct
   */
  var CastMode = function(type, description, host, isForced) {
    /** @type {number} */
    this.type = type;

    /** @type {string} */
    this.description = description;

    /** @type {?string} */
    this.host = host || null;

    /** @type {boolean} */
    this.isForced = isForced;
  };

  /**
   * Placeholder object for AUTO cast mode. See comment in CastModeType.
   * @const {!media_router.CastMode}
   */
  var AUTO_CAST_MODE = new CastMode(
      media_router.CastModeType.AUTO, loadTimeData.getString('autoCastMode'),
      null, false);

  /**
   * @param {number} id The ID of this issue.
   * @param {string} title The issue title.
   * @param {string} message The issue message.
   * @param {number} defaultActionType The type of default action.
   * @param {number|undefined} secondaryActionType The type of optional action.
   * @param {?string} routeId The route ID to which this issue
   *                  pertains. If not set, this is a global issue.
   * @param {boolean} isBlocking True if this issue blocks other UI.
   * @param {?number} helpPageId The numeric help center ID.
   * @constructor
   * @struct
   */
  var Issue = function(
      id, title, message, defaultActionType, secondaryActionType, routeId,
      isBlocking, helpPageId) {
    /** @type {number} */
    this.id = id;

    /** @type {string} */
    this.title = title;

    /** @type {string} */
    this.message = message;

    /** @type {number} */
    this.defaultActionType = defaultActionType;

    /** @type {number|undefined} */
    this.secondaryActionType = secondaryActionType;

    /** @type {?string} */
    this.routeId = routeId;

    /** @type {boolean} */
    this.isBlocking = isBlocking;

    /** @type {?number} */
    this.helpPageId = helpPageId;
  };

  /**
   * @param {string} id The media route ID.
   * @param {string} sinkId The ID of the media sink running this route.
   * @param {string} description The short description of this route.
   * @param {?number} tabId The ID of the tab in which web app is running and
   *                  accessing the route.
   * @param {boolean} isLocal True if this is a locally created route.
   * @param {boolean} canJoin True if this route can be joined.
   * @param {?string} customControllerPath non-empty if this route has custom
   *                  controller.
   * @constructor
   * @struct
   */
  var Route = function(
      id, sinkId, description, tabId, isLocal, canJoin, customControllerPath) {
    /** @type {string} */
    this.id = id;

    /** @type {string} */
    this.sinkId = sinkId;

    /** @type {string} */
    this.description = description;

    /** @type {?number} */
    this.tabId = tabId;

    /** @type {boolean} */
    this.isLocal = isLocal;

    /** @type {boolean} */
    this.canJoin = canJoin;

    /** @type {number|undefined} */
    this.currentCastMode = undefined;

    /** @type {?string} */
    this.customControllerPath = customControllerPath;

    /** @type {boolean} */
    this.supportsWebUiController = false;
  };

  /**
   * @param {string} title The title of the route.
   * @param {boolean} canPlayPause Whether the route can be played/paused.
   * @param {boolean} canMute Whether the route can be muted/unmuted.
   * @param {boolean} canSetVolume Whether the route volume can be changed.
   * @param {boolean} canSeek Whether the route's playback position can be
   *     changed.
   * @param {boolean} isPaused Whether the route is paused.
   * @param {boolean} isMuted Whether the route is muted.
   * @param {number} volume The route's volume, between 0 and 1.
   * @param {number} duration The route's duration in seconds.
   * @param {number} currentTime The route's current position in seconds.
   *     Must not be greater than |duration|.
   * @param {!{mediaRemotingEnabled: boolean}=} mirroringExtraData Only set for
   *     mirroring routes.
   * @param {!{localPresent: boolean}=} hangoutsExtraData Only set for Hangouts
   *     routes.
   * @constructor
   * @struct
   */
  var RouteStatus = function(
      title = '', canPlayPause = false, canMute = false, canSetVolume = false,
      canSeek = false, playState = media_router.PlayState.PLAYING,
      isPaused = false, isMuted = false, volume = 0, duration = 0,
      currentTime = 0, hangoutsExtraData = undefined,
      mirroringExtraData = undefined) {

    /** @type {string} */
    this.title = title;

    /** @type {boolean} */
    this.canPlayPause = canPlayPause;

    /** @type {boolean} */
    this.canMute = canMute;

    /** @type {boolean} */
    this.canSetVolume = canSetVolume;

    /** @type {boolean} */
    this.canSeek = canSeek;

    /** @type {media_router.PlayState} */
    this.playState = playState;

    /** @type {boolean} */
    this.isMuted = isMuted;

    /** @type {number} */
    this.volume = volume;

    /** @type {number} */
    this.duration = duration;

    /** @type {number} */
    this.currentTime = currentTime;

    /** @type {!{localPresent: boolean}|undefined} */
    this.hangoutsExtraData = hangoutsExtraData;

    /** @type {!{mediaRemotingEnabled: boolean}|undefined} */
    this.mirroringExtraData = mirroringExtraData;
  };

  /**
   * @param {string} id The ID of the media sink.
   * @param {string} name The name of the sink.
   * @param {?string} description Optional description of the sink.
   * @param {?string} domain Optional domain of the sink.
   * @param {media_router.SinkIconType} iconType the type of icon for the sink.
   * @param {media_router.SinkStatus} status The readiness state of the sink.
   * @param {number} castModes Bitset of cast modes compatible with the sink.
   * @constructor
   * @struct
   */
  var Sink = function(
      id, name, description, domain, iconType, status, castModes) {
    /** @type {string} */
    this.id = id;

    /** @type {string} */
    this.name = name;

    /** @type {?string} */
    this.description = description;

    /** @type {?string} */
    this.domain = domain;

    /** @type {!media_router.SinkIconType} */
    this.iconType = iconType;

    /** @type {!media_router.SinkStatus} */
    this.status = status;

    /** @type {number} */
    this.castModes = castModes;

    /** @type {boolean} */
    this.isPseudoSink = false;
  };

  /**
   * @param {number} tabId The current tab ID.
   * @param {string} domain The domain of the current tab.
   * @constructor
   * @struct
   */
  var TabInfo = function(tabId, domain) {
    /** @type {number} */
    this.tabId = tabId;

    /** @type {string} */
    this.domain = domain;
  };

  return {
    AUTO_CAST_MODE: AUTO_CAST_MODE,
    CastMode: CastMode,
    Issue: Issue,
    Route: Route,
    RouteStatus: RouteStatus,
    Sink: Sink,
    TabInfo: TabInfo,
  };
});
