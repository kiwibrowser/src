// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * A class of server log entries.
 *
 * Any changes to the values here need to be coordinated with the host and
 * server/log proto code.
 * See remoting/signaling/server_log_entry.{cc|h}
 */

'use strict';

/** @suppress {duplicate} */
var remoting = remoting || {};

/**
 * @private
 * @constructor
 */
remoting.ServerLogEntry = function() {
  /** @type Object<string> */ this.dict = {};
};

/** @private */
remoting.ServerLogEntry.KEY_EVENT_NAME_ = 'event-name';
/** @private */
remoting.ServerLogEntry.VALUE_EVENT_NAME_SESSION_STATE_ = 'session-state';
/** @private */
remoting.ServerLogEntry.VALUE_EVENT_NAME_SIGNAL_STRATEGY_PROGRESS_ =
    'signal-strategy-progress';
/** @private */
remoting.ServerLogEntry.KEY_SESSION_ID_ = 'session-id';
/** @private */
remoting.ServerLogEntry.KEY_ROLE_ = 'role';
/** @private */
remoting.ServerLogEntry.VALUE_ROLE_CLIENT_ = 'client';
/** @private */
remoting.ServerLogEntry.KEY_SESSION_STATE_ = 'session-state';
/** @private */
remoting.ServerLogEntry.KEY_CONNECTION_TYPE_ = 'connection-type';
/** @private */
remoting.ServerLogEntry.KEY_SIGNAL_STRATEGY_TYPE_ = 'signal-strategy-type';
/** @private */
remoting.ServerLogEntry.KEY_SIGNAL_STRATEGY_PROGRESS_ =
    'signal-strategy-progress';
/** @private */
remoting.ServerLogEntry.KEY_SESSION_DURATION_SECONDS_ = 'session-duration';

/** @private */
remoting.ServerLogEntry.KEY_XMPP_ERROR_RAW_STANZA = 'xmpp-error-raw-stanza';

/**
 * @private
 * @param {remoting.ClientSession.State} state
 * @return {string}
 */
remoting.ServerLogEntry.getValueForSessionState_ = function(state) {
  switch(state) {
    case remoting.ClientSession.State.UNKNOWN:
      return 'unknown';
    case remoting.ClientSession.State.INITIALIZING:
      return 'initializing';
    case remoting.ClientSession.State.CONNECTING:
      return 'connecting';
    case remoting.ClientSession.State.AUTHENTICATED:
      return 'authenticated';
    case remoting.ClientSession.State.CONNECTED:
      return 'connected';
    case remoting.ClientSession.State.CLOSED:
      return 'closed';
    case remoting.ClientSession.State.FAILED:
      return 'connection-failed';
    case remoting.ClientSession.State.CONNECTION_DROPPED:
      return 'connection-dropped';
    case remoting.ClientSession.State.CONNECTION_CANCELED:
      return 'connection-canceled';
    default:
      return 'undefined-' + state;
  }
};

/** @private */
remoting.ServerLogEntry.KEY_CONNECTION_ERROR_ = 'connection-error';

/**
 * @private
 * @param {!remoting.Error} connectionError
 * @return {string}
 */
remoting.ServerLogEntry.getValueForError_ = function(connectionError) {
  // Directory service should be updated if a new string is added here as
  // otherwise the error code will be ignored (i.e. recorded as 0 instead).
  switch (connectionError.getTag()) {
    case remoting.Error.Tag.NONE:
      return 'none';
    case remoting.Error.Tag.INVALID_ACCESS_CODE:
      return 'invalid-access-code';
    case remoting.Error.Tag.MISSING_PLUGIN:
      return 'missing_plugin';
    case remoting.Error.Tag.AUTHENTICATION_FAILED:
      return 'authentication-failed';
    case remoting.Error.Tag.HOST_IS_OFFLINE:
      return 'host-is-offline';
    case remoting.Error.Tag.INCOMPATIBLE_PROTOCOL:
      return 'incompatible-protocol';
    case remoting.Error.Tag.BAD_VERSION:
      return 'bad-plugin-version';
    case remoting.Error.Tag.NETWORK_FAILURE:
      return 'network-failure';
    case remoting.Error.Tag.HOST_OVERLOAD:
      return 'host-overload';
    case remoting.Error.Tag.P2P_FAILURE:
      return 'p2p-failure';
    case remoting.Error.Tag.CLIENT_SUSPENDED:
      return 'client-suspended';
    case remoting.Error.Tag.MAX_SESSION_LENGTH:
      return 'max-session-length';
    case remoting.Error.Tag.HOST_CONFIGURATION_ERROR:
      return 'host-configuration-error';
    case remoting.Error.Tag.UNEXPECTED:
      return 'unexpected';
    default:
      return 'unknown-' + connectionError.getTag();
  }
};

/** @private */
remoting.ServerLogEntry.VALUE_EVENT_NAME_CONNECTION_STATISTICS_ =
    "connection-statistics";
/** @private */
remoting.ServerLogEntry.KEY_VIDEO_BANDWIDTH_ = "video-bandwidth";
/** @private */
remoting.ServerLogEntry.KEY_CAPTURE_LATENCY_ = "capture-latency";
/** @private */
remoting.ServerLogEntry.KEY_ENCODE_LATENCY_ = "encode-latency";
/** @private */
remoting.ServerLogEntry.KEY_DECODE_LATENCY_ = "decode-latency";
/** @private */
remoting.ServerLogEntry.KEY_RENDER_LATENCY_ = "render-latency";
/** @private */
remoting.ServerLogEntry.KEY_ROUNDTRIP_LATENCY_ = "roundtrip-latency";

/** @private */
remoting.ServerLogEntry.KEY_OS_NAME_ = 'os-name';

/** @private */
remoting.ServerLogEntry.KEY_OS_VERSION_ = 'os-version';

/** @private */
remoting.ServerLogEntry.KEY_CPU_ = 'cpu';

/** @private */
remoting.ServerLogEntry.KEY_BROWSER_VERSION_ = 'browser-version';

/** @private */
remoting.ServerLogEntry.KEY_WEBAPP_VERSION_ = 'webapp-version';

/** @private */
remoting.ServerLogEntry.KEY_HOST_VERSION_ = 'host-version';

/** @private */
remoting.ServerLogEntry.VALUE_EVENT_NAME_SESSION_ID_OLD_ = 'session-id-old';

/** @private */
remoting.ServerLogEntry.VALUE_EVENT_NAME_SESSION_ID_NEW_ = 'session-id-new';

/** @private */
remoting.ServerLogEntry.KEY_MODE_ = 'mode';
// These values are passed in by the Activity to identify the current mode.
remoting.ServerLogEntry.VALUE_MODE_IT2ME = 'it2me';
remoting.ServerLogEntry.VALUE_MODE_ME2ME = 'me2me';
remoting.ServerLogEntry.VALUE_MODE_APP_REMOTING = 'lgapp';
remoting.ServerLogEntry.VALUE_MODE_UNKNOWN = 'unknown';

/** @private */
remoting.ServerLogEntry.KEY_APP_ID_ = 'application-id';

/**
 * Sets one field in this log entry.
 *
 * @private
 * @param {string} key
 * @param {string} value
 */
remoting.ServerLogEntry.prototype.set_ = function(key, value) {
  this.dict[key] = value;
};

/**
 * Converts this object into an XML stanza.
 *
 * @return {string}
 */
remoting.ServerLogEntry.prototype.toStanza = function() {
  var stanza = '<gr:entry ';
  for (var key in this.dict) {
    stanza += escape(key) + '="' + escape(this.dict[key]) + '" ';
  }
  stanza += '/>';
  return stanza;
};

/**
 * Prints this object on the debug log.
 *
 * @param {number} indentLevel the indentation level
 */
remoting.ServerLogEntry.prototype.toDebugLog = function(indentLevel) {
  /** @type Array<string> */ var fields = [];
  for (var key in this.dict) {
    fields.push(key + ': ' + this.dict[key]);
  }
  console.log(Array(indentLevel+1).join("  ") + fields.join(', '));
};

/**
 * Makes a log entry for a change of client session state.
 *
 * @param {remoting.ClientSession.State} state
 * @param {!remoting.Error} connectionError
 * @param {string} mode The current app mode (It2Me, Me2Me, AppRemoting).
 * @param {string} role 'client' if the app is acting as a Chromoting client
 *     or 'host' if it is acting as a host (IT2Me)
 * @return {remoting.ServerLogEntry}
 */
remoting.ServerLogEntry.makeClientSessionStateChange = function(state,
    connectionError, mode, role) {
  var entry = new remoting.ServerLogEntry();
  entry.set_(remoting.ServerLogEntry.KEY_ROLE_, role);
  entry.set_(remoting.ServerLogEntry.KEY_EVENT_NAME_,
             remoting.ServerLogEntry.VALUE_EVENT_NAME_SESSION_STATE_);
  entry.set_(remoting.ServerLogEntry.KEY_SESSION_STATE_,
             remoting.ServerLogEntry.getValueForSessionState_(state));
  if (!connectionError.isNone()) {
    entry.set_(remoting.ServerLogEntry.KEY_CONNECTION_ERROR_,
               remoting.ServerLogEntry.getValueForError_(connectionError));
  }
  entry.addModeField(mode);
  return entry;
};

/**
 * Makes a log entry for a set of connection statistics.
 * Returns null if all the statistics were zero.
 *
 * @param {remoting.StatsAccumulator} statsAccumulator
 * @param {string} connectionType
 * @param {string} mode The current app mode (It2Me, Me2Me, AppRemoting).
 * @return {?remoting.ServerLogEntry}
 */
remoting.ServerLogEntry.makeStats = function(statsAccumulator,
                                             connectionType, mode) {
  var entry = new remoting.ServerLogEntry();
  entry.set_(remoting.ServerLogEntry.KEY_ROLE_,
             remoting.ServerLogEntry.VALUE_ROLE_CLIENT_);
  entry.set_(remoting.ServerLogEntry.KEY_EVENT_NAME_,
             remoting.ServerLogEntry.VALUE_EVENT_NAME_CONNECTION_STATISTICS_);
  if (connectionType) {
    entry.set_(remoting.ServerLogEntry.KEY_CONNECTION_TYPE_,
               connectionType);
  }
  entry.addModeField(mode);
  var nonZero = false;
  nonZero |= entry.addStatsField_(
      remoting.ServerLogEntry.KEY_VIDEO_BANDWIDTH_,
      remoting.ClientSession.STATS_KEY_VIDEO_BANDWIDTH, statsAccumulator);
  nonZero |= entry.addStatsField_(
      remoting.ServerLogEntry.KEY_CAPTURE_LATENCY_,
      remoting.ClientSession.STATS_KEY_CAPTURE_LATENCY, statsAccumulator);
  nonZero |= entry.addStatsField_(
      remoting.ServerLogEntry.KEY_ENCODE_LATENCY_,
      remoting.ClientSession.STATS_KEY_ENCODE_LATENCY, statsAccumulator);
  nonZero |= entry.addStatsField_(
      remoting.ServerLogEntry.KEY_DECODE_LATENCY_,
      remoting.ClientSession.STATS_KEY_DECODE_LATENCY, statsAccumulator);
  nonZero |= entry.addStatsField_(
      remoting.ServerLogEntry.KEY_RENDER_LATENCY_,
      remoting.ClientSession.STATS_KEY_RENDER_LATENCY, statsAccumulator);
  nonZero |= entry.addStatsField_(
      remoting.ServerLogEntry.KEY_ROUNDTRIP_LATENCY_,
      remoting.ClientSession.STATS_KEY_ROUNDTRIP_LATENCY, statsAccumulator);
  if (nonZero) {
    return entry;
  }
  return null;
};

/**
 * Adds one connection statistic to a log entry.
 *
 * @private
 * @param {string} entryKey
 * @param {string} statsKey
 * @param {remoting.StatsAccumulator} statsAccumulator
 * @return {boolean} whether the statistic is non-zero
 */
remoting.ServerLogEntry.prototype.addStatsField_ = function(
    entryKey, statsKey, statsAccumulator) {
  var val = statsAccumulator.calcMean(statsKey);
  this.set_(entryKey, val.toFixed(2));
  return (val != 0);
};

/**
 * Makes a log entry for a "this session ID is old" event.
 *
 * @param {string} sessionId
 * @param {string} mode The current app mode (It2Me, Me2Me, AppRemoting).
 * @return {remoting.ServerLogEntry}
 */
remoting.ServerLogEntry.makeSessionIdOld = function(sessionId, mode) {
  var entry = new remoting.ServerLogEntry();
  entry.set_(remoting.ServerLogEntry.KEY_ROLE_,
             remoting.ServerLogEntry.VALUE_ROLE_CLIENT_);
  entry.set_(remoting.ServerLogEntry.KEY_EVENT_NAME_,
             remoting.ServerLogEntry.VALUE_EVENT_NAME_SESSION_ID_OLD_);
  entry.addSessionIdField(sessionId);
  entry.addModeField(mode);
  return entry;
};

/**
 * Makes a log entry for a "this session ID is new" event.
 *
 * @param {string} sessionId
 * @param {string} mode The current app mode (It2Me, Me2Me, AppRemoting).
 * @return {remoting.ServerLogEntry}
 */
remoting.ServerLogEntry.makeSessionIdNew = function(sessionId, mode) {
  var entry = new remoting.ServerLogEntry();
  entry.set_(remoting.ServerLogEntry.KEY_ROLE_,
             remoting.ServerLogEntry.VALUE_ROLE_CLIENT_);
  entry.set_(remoting.ServerLogEntry.KEY_EVENT_NAME_,
             remoting.ServerLogEntry.VALUE_EVENT_NAME_SESSION_ID_NEW_);
  entry.addSessionIdField(sessionId);
  entry.addModeField(mode);
  return entry;
};

/**
 * Makes a log entry for a "signal strategy fallback" event.
 *
 * @param {string} sessionId
 * @param {remoting.SignalStrategy.Type} strategyType
 * @param {remoting.FallbackSignalStrategy.Progress} progress
 * @return {remoting.ServerLogEntry}
 */
remoting.ServerLogEntry.makeSignalStrategyProgress =
    function(sessionId, strategyType, progress) {
  var entry = new remoting.ServerLogEntry();
  entry.set_(remoting.ServerLogEntry.KEY_ROLE_,
             remoting.ServerLogEntry.VALUE_ROLE_CLIENT_);
  entry.set_(
      remoting.ServerLogEntry.KEY_EVENT_NAME_,
      remoting.ServerLogEntry.VALUE_EVENT_NAME_SIGNAL_STRATEGY_PROGRESS_);
  entry.addSessionIdField(sessionId);
  entry.set_(remoting.ServerLogEntry.KEY_SIGNAL_STRATEGY_TYPE_, strategyType);
  entry.set_(remoting.ServerLogEntry.KEY_SIGNAL_STRATEGY_PROGRESS_, progress);

  return entry;
};

/**
 * Adds a session ID field to this log entry.
 *
 * @param {string} sessionId
 */
remoting.ServerLogEntry.prototype.addSessionIdField = function(sessionId) {
  this.set_(remoting.ServerLogEntry.KEY_SESSION_ID_, sessionId);
}

/**
 * Adds fields describing the host to this log entry.
 */
remoting.ServerLogEntry.prototype.addClientOSFields = function() {
  var systemInfo = remoting.getSystemInfo();
  if (systemInfo) {
    if (systemInfo.osName.length > 0) {
      this.set_(remoting.ServerLogEntry.KEY_OS_NAME_, systemInfo.osName);
    }
    if (systemInfo.osVersion.length > 0) {
      this.set_(remoting.ServerLogEntry.KEY_OS_VERSION_, systemInfo.osVersion);
    }
    if (systemInfo.cpu.length > 0) {
      this.set_(remoting.ServerLogEntry.KEY_CPU_, systemInfo.cpu);
    }
  }
};

/**
 * Adds a field to this log entry specifying the time in seconds since the start
 * of the session to the current event.
 * @param {number} sessionDurationInSeconds
 */
remoting.ServerLogEntry.prototype.addSessionDuration =
    function(sessionDurationInSeconds) {
  this.set_(remoting.ServerLogEntry.KEY_SESSION_DURATION_SECONDS_,
            String(sessionDurationInSeconds));
};


/**
 * Adds a field specifying the browser version to this log entry.
 */
remoting.ServerLogEntry.prototype.addChromeVersionField = function() {
  var version = remoting.getChromeVersion();
  if (version != null) {
    this.set_(remoting.ServerLogEntry.KEY_BROWSER_VERSION_, version);
  }
};

/**
 * Adds a field specifying the webapp version to this log entry.
 */
remoting.ServerLogEntry.prototype.addWebappVersionField = function() {
  var manifest = chrome.runtime.getManifest();
  if (manifest && manifest.version) {
    this.set_(remoting.ServerLogEntry.KEY_WEBAPP_VERSION_, manifest.version);
  }
};

/**
 * Adds a field specifying the host version to this log entry.
 * @param {string} hostVersion Version of the host for current session.
 * @return {void} Nothing.
 */
remoting.ServerLogEntry.prototype.addHostVersion = function(hostVersion) {
  this.set_(remoting.ServerLogEntry.KEY_HOST_VERSION_, hostVersion);
};

/**
 * Stub.
 * @param {remoting.ChromotingEvent.Os} hostOs type of the host OS for current
 *        session.
 * @return {void} Nothing.
 */
remoting.ServerLogEntry.prototype.addHostOs = function(hostOs) {};

/**
 * Stub.
 * @param {string} hostOsVersion Version of the host OS for current session.
 * @return {void} Nothing.
 */
remoting.ServerLogEntry.prototype.addHostOsVersion = function(hostOsVersion) {};

/**
 * Adds a field specifying the mode to this log entry.
 * @param {string} mode The current app mode (It2Me, Me2Me, AppRemoting).
 */
remoting.ServerLogEntry.prototype.addModeField = function(mode) {
  this.set_(remoting.ServerLogEntry.KEY_MODE_, mode);
};

/**
 * Adds a field specifying the application ID to this log entry.
 * @return {void} Nothing.
 */
remoting.ServerLogEntry.prototype.addApplicationId = function() {
  this.set_(remoting.ServerLogEntry.KEY_APP_ID_, chrome.runtime.id);
};

/**
 * Adds a field specifying the XMPP error to this log entry.
 * @param {?remoting.ChromotingEvent.XmppError} xmppError
 * @return {void} Nothing.
 */
remoting.ServerLogEntry.prototype.addXmppError = function(xmppError) {
  if (!Boolean(xmppError)) {
    return;
  }
  this.set_(remoting.ServerLogEntry.KEY_XMPP_ERROR_RAW_STANZA,
            xmppError.raw_stanza);
};
