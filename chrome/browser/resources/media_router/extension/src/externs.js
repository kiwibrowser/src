// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// NOTE: Througout this file, we use constructors instead of @typedefs to
// declare object properties.  This prevents the JSCompiler from renaming these
// properties.  These can be converted to shorter @typedef declarations when the
// JSCompiler adds full support for @typedef in externs.

//////////////////////////////////////////////////////////////////////////////
// Externs for WebRTC PeerConnection as in
// http://www.w3.org/TR/2012/WD-webrtc-20120821/
//////////////////////////////////////////////////////////////////////////////


/** @type {string} */
MediaStreamEvent.prototype.type;


/** @type {string} */
RTCPeerConnection.prototype.iceConnectionState;

//////////////////////////////////////////////////////////////////////////////
// Externs for processes API
// See: https://developer.chrome.com/extensions/processes
//////////////////////////////////////////////////////////////////////////////


/**
 * @const
 */
chrome.processes = {};


/**
 * @param {number} tabId
 * @param {Function} callback
 */
chrome.processes.getProcessIdForTab = function(tabId, callback) {};


/**
 * @const
 */
chrome.processes.onUpdated = {};


/**
 * @param {Function} callback
 */
chrome.processes.onUpdated.addListener = function(callback) {};


/**
 * @param {Function} callback
 */
chrome.processes.onUpdated.removeListener = function(callback) {};

//////////////////////////////////////////////////////////////////////////////
// Externs for Tab Capture API
// See: https://developer.chrome.com/extensions/tabCapture.html
//////////////////////////////////////////////////////////////////////////////


/** @const {*} */
chrome.tabCapture = {};


/**
 * @param {MediaConstraints} constraints
 * @param {function(MediaStream)} callback
 */
chrome.tabCapture.capture = function(constraints, callback) {};


/**
 * @param {string} startUrl
 * @param {MediaConstraints} constraints
 * @param {function(MediaStream)} callback
 */
chrome.tabCapture.captureOffscreenTab = function(
    startUrl, constraints, callback) {};


/**
 * @type {ChromeEvent}
 */
chrome.tabCapture.onStatusChanged;

//////////////////////////////////////////////////////////////////////////////
// Externs for Desktop Capture API
// See: https://developer.chrome.com/extensions/desktopCapture.html
//////////////////////////////////////////////////////////////////////////////


/** @const */
chrome.desktopCapture = {};


/**
 * @param {Array<string>} sources
 * @param {function(string)} callback
 * @return {number} desktopMediaRequestId
 */
chrome.desktopCapture.chooseDesktopMedia = function(sources, callback) {};


/**
 * @param {number} desktopMediaRequestId
 */
chrome.desktopCapture.cancelChooseDesktopMedia = function(
    desktopMediaRequestId) {};


//////////////////////////////////////////////////////////////////////////////
// Externs for the chrome.cast.channel API
// IDL: http://goo.gl/G1hmAI
//////////////////////////////////////////////////////////////////////////////


/** @const */
chrome.cast = chrome.cast || {};


/** @const */
chrome.cast.channel = {};



/** @constructor */
chrome.cast.channel.ChannelInfo = function() {};


/** @type {number} */
chrome.cast.channel.ChannelInfo.prototype.channelId;


/** @type {string} */
chrome.cast.channel.ChannelInfo.prototype.readyState;


/** @type {?string} */
chrome.cast.channel.ChannelInfo.prototype.errorState;


/** @type {?boolean} */
chrome.cast.channel.ChannelInfo.prototype.keepAlive;


/** @type {?boolean} */
chrome.cast.channel.ChannelInfo.prototype.audioOnly;


/** @type {!chrome.cast.channel.ConnectInfo} */
chrome.cast.channel.ChannelInfo.prototype.connectInfo;



/** @constructor */
chrome.cast.channel.MessageInfo = function() {};


/** @type {string} */
chrome.cast.channel.MessageInfo.prototype.namespace_;


/** @type {*} */
chrome.cast.channel.MessageInfo.prototype.data;


/** @type {string} */
chrome.cast.channel.MessageInfo.prototype.sourceId;


/** @type {string} */
chrome.cast.channel.MessageInfo.prototype.destinationId;



/** @constructor */
chrome.cast.channel.ConnectInfo = function() {};


/** @type {string} */
chrome.cast.channel.ConnectInfo.prototype.ipAddress;


/** @type {number} */
chrome.cast.channel.ConnectInfo.prototype.port;


/** @type {string} */
chrome.cast.channel.ConnectInfo.prototype.auth;


/** @type {number} */
chrome.cast.channel.ConnectInfo.prototype.timeout;


/** @type {number} */
chrome.cast.channel.ConnectInfo.prototype.livenessTimeout;


/** @type {number} */
chrome.cast.channel.ConnectInfo.prototype.pingInterval;



/** @constructor */
chrome.cast.channel.ErrorInfo = function() {};


/** @type {string} */
chrome.cast.channel.ErrorInfo.prototype.errorState;


/** @type {?number} */
chrome.cast.channel.ErrorInfo.prototype.eventType;


/** @type {?number} */
chrome.cast.channel.ErrorInfo.prototype.challengeReplyErrorType;


/** @type {?number} */
chrome.cast.channel.ErrorInfo.prototype.netReturnValue;


/** @type {?number} */
chrome.cast.channel.ErrorInfo.prototype.nssErrorCode;


/**
 * @param {!chrome.cast.channel.ConnectInfo} connectInfo
 * @param {function(!chrome.cast.channel.ChannelInfo=)} callback
 */
chrome.cast.channel.open = function(connectInfo, callback) {};


/**
 * @param {!chrome.cast.channel.ChannelInfo} channel
 * @param {!chrome.cast.channel.MessageInfo} message
 * @param {function(!chrome.cast.channel.ChannelInfo=)} callback
 */
chrome.cast.channel.send = function(channel, message, callback) {};


/**
 * @param {!chrome.cast.channel.ChannelInfo} channel
 * @param {function(!chrome.cast.channel.ChannelInfo=)} callback
 */
chrome.cast.channel.close = function(channel, callback) {};



/** @const */
chrome.cast.channel.onMessage;


/**
 * @param {!function(!chrome.cast.channel.ChannelInfo,
 *     !chrome.cast.channel.MessageInfo)} listener
 */
chrome.cast.channel.onMessage.addListener = function(listener) {};


/**
 * @param {!function(!chrome.cast.channel.ChannelInfo,
 *     !chrome.cast.channel.MessageInfo)} listener
 */
chrome.cast.channel.onMessage.removeListener = function(listener) {};


/** @const */
chrome.cast.channel.onError;


/**
 * @param {!function(!chrome.cast.channel.ChannelInfo,
 *     (!chrome.cast.channel.ErrorInfo|undefined))} listener
 */
chrome.cast.channel.onError.addListener = function(listener) {};


/**
 * @param {!function(!chrome.cast.channel.ChannelInfo,
 *     (!chrome.cast.channel.ErrorInfo|undefined))} listener
 */
chrome.cast.channel.onError.removeListener = function(listener) {};


/** @const */
chrome.cast.media = {};


/**
 * @param {function(ChromeWindow)} callback
 */
chrome.browserAction.openPopup = function(callback) {};

//////////////////////////////////////////////////////////////////////////////
// Externs for the chrome.cast.streaming APIs
// See: http://goo.gl/yInHUU
//////////////////////////////////////////////////////////////////////////////


/** @const {*} */
chrome.cast.streaming = {};


/** @const {*} */
chrome.cast.streaming.session = {};


/**
 * @param {?MediaStreamTrack} audio
 * @param {?MediaStreamTrack} video
 * @param {!function(?number, ?number, !number)} callback
 */
chrome.cast.streaming.session.create = function(audio, video, callback) {};


/** @const {*} */
chrome.cast.streaming.rtpStream = {};



/** @constructor */
chrome.cast.streaming.rtpStream.CodecSpecificParams = function() {};



/** @constructor */
chrome.cast.streaming.rtpStream.RtpPayloadParams = function() {};


/** @type {number} */
chrome.cast.streaming.rtpStream.RtpPayloadParams.prototype.maxLatency;


/** @type {number} */
chrome.cast.streaming.rtpStream.RtpPayloadParams.prototype.minLatency;


/** @type {number} */
chrome.cast.streaming.rtpStream.RtpPayloadParams.prototype.animatedLatency;


/** @type {number} */
chrome.cast.streaming.rtpStream.RtpPayloadParams.prototype.payloadType;


/** @type {string} */
chrome.cast.streaming.rtpStream.RtpPayloadParams.prototype.codecName;


/** @type {number} */
chrome.cast.streaming.rtpStream.RtpPayloadParams.prototype.ssrc;


/** @type {number} */
chrome.cast.streaming.rtpStream.RtpPayloadParams.prototype.feedbackSsrc;


/** @type {number} */
chrome.cast.streaming.rtpStream.RtpPayloadParams.prototype.clockRate;


/** @type {number} */
chrome.cast.streaming.rtpStream.RtpPayloadParams.prototype.minBitrate;


/** @type {number} */
chrome.cast.streaming.rtpStream.RtpPayloadParams.prototype.maxBitrate;


/** @type {number} */
chrome.cast.streaming.rtpStream.RtpPayloadParams.prototype.channels;


/** @type {number} */
chrome.cast.streaming.rtpStream.RtpPayloadParams.prototype.maxFrameRate;


/** @type {number} */
chrome.cast.streaming.rtpStream.RtpPayloadParams.prototype.width;


/** @type {number} */
chrome.cast.streaming.rtpStream.RtpPayloadParams.prototype.height;


/** @type {string} */
chrome.cast.streaming.rtpStream.RtpPayloadParams.prototype.aesKey;


/** @type {string} */
chrome.cast.streaming.rtpStream.RtpPayloadParams.prototype.aesIvMask;


/** @type {Array.<chrome.cast.streaming.rtpStream.CodecSpecificParams>} */
chrome.cast.streaming.rtpStream.RtpPayloadParams.prototype.codecSpecificParams;



/** @constructor */
chrome.cast.streaming.rtpStream.RtpParams = function() {};


/** @type {chrome.cast.streaming.rtpStream.RtpPayloadParams} */
chrome.cast.streaming.rtpStream.RtpParams.prototype.payload;


/** @type {Array.<string>} */
chrome.cast.streaming.rtpStream.RtpParams.prototype.rtcpFeatures;


/**
 * @param {!number} streamId
 */
chrome.cast.streaming.rtpStream.destroy = function(streamId) {};


/**
 * @param {!number} streamId
 * @return {!Array.<!chrome.cast.streaming.rtpStream.RtpParams>}
 */
chrome.cast.streaming.rtpStream.getSupportedParams = function(streamId) {
  return [new chrome.cast.streaming.rtpStream.RtpParams];
};


/**
 * @param {!number} streamId
 * @param {!chrome.cast.streaming.rtpStream.RtpParams} params
 */
chrome.cast.streaming.rtpStream.start = function(streamId, params) {};


/**
 * @param {!number} streamId
 */
chrome.cast.streaming.rtpStream.stop = function(streamId) {};


/**
 * @param {!number} streamId
 * @param {!boolean} enable
 */
chrome.cast.streaming.rtpStream.toggleLogging = function(streamId, enable) {};


/**
 * @param {!number} streamId
 * @param {!string} extraData
 * @param {!function(ArrayBuffer)} callback
 */
chrome.cast.streaming.rtpStream.getRawEvents = function(
    streamId, extraData, callback) {};


/**
 * @param {!number} streamId
 * @param {!function(Object)} callback
 */
chrome.cast.streaming.rtpStream.getStats = function(streamId, callback) {};


/** @const {*} */
chrome.cast.streaming.rtpStream.onStarted = {};


/**
 * @param {!function(!number)} listener
 */
chrome.cast.streaming.rtpStream.onStarted.addListener = function(listener) {};


/**
 * @param {!function(!number)} listener
 */
chrome.cast.streaming.rtpStream.onStarted.removeListener = function(listener) {
};


/** @const {*} */
chrome.cast.streaming.rtpStream.onStopped = {};


/**
 * @param {!function(!number)} listener
 */
chrome.cast.streaming.rtpStream.onStopped.addListener = function(listener) {};


/**
 * @param {!function(!number)} listener
 */
chrome.cast.streaming.rtpStream.onStopped.removeListener = function(listener) {
};


/** @const {*} */
chrome.cast.streaming.rtpStream.onError = {};


/**
 * @param {!function(number, string)} listener
 */
chrome.cast.streaming.rtpStream.onError.addListener = function(listener) {};


/**
 * @param {!function(number, string)} listener
 */
chrome.cast.streaming.rtpStream.onError.removeListener = function(listener) {};



/** @constructor */
chrome.cast.streaming.udpTransport.IPEndPoint = function() {};


/** @type {string} */
chrome.cast.streaming.udpTransport.IPEndPoint.prototype.address;


/** @type {number} */
chrome.cast.streaming.udpTransport.IPEndPoint.prototype.port;


/**
 * @param {!number} streamId
 */
chrome.cast.streaming.udpTransport.destroy = function(streamId) {};


/** @const {*} */
chrome.cast.streaming.udpTransport = {};


/**
 * @param {!number} transportId
 * @param {!chrome.cast.streaming.udpTransport.IPEndPoint} ipEndPoint
 */
chrome.cast.streaming.udpTransport.setDestination = function(
    transportId, ipEndPoint) {};


/**
 * @param {!number} transportId
 * @param {!Object} options
 */
chrome.cast.streaming.udpTransport.setOptions = function(transportId, options) {
};


/** @const {*} */
chrome.mojoPrivate = {};


/**
 * @param {!string} moduleName
 * @return {!Promise.<T>}
 * @template T
 */
chrome.mojoPrivate.requireAsync = function(moduleName) {
  return Promise.resolve(Object.create(null));
};


//////////////////////////////////////////////////////////////////////////////
// Externs for UMA analytics
//////////////////////////////////////////////////////////////////////////////


/** @const {*} */
chrome.metricsPrivate = {};


/**
 * Records an elapsed time of no more than 10 seconds.  The sample value is
 * specified in milliseconds.
 * @param {string} metricName
 * @param {number} value
 */
chrome.metricsPrivate.recordTime = function(metricName, value) {};


/**
 * Records an elapsed time of no more than 3 minutes.  The sample value is
 * specified in milliseconds.
 * @param {string} metricName
 * @param {number} value
 */
chrome.metricsPrivate.recordMediumTime = function(metricName, value) {};


/**
 * Records an elapsed time of no more than 1 hour.  The sample value is
 * specified in milliseconds.
 * @param {string} metricName
 * @param {number} value
 */
chrome.metricsPrivate.recordLongTime = function(metricName, value) {};


/**
 * Records an action performed by the user.
 * @param {string} name
 */
chrome.metricsPrivate.recordUserAction = function(name) {};


/**
 * Records a value than can range from 1 to 100.
 * @param {string} metricName
 * @param {number} count
 */
chrome.metricsPrivate.recordSmallCount = function(metricName, count) {};


/**
 * Returns true if the user opted in to sending crash reports.
 * @param {!function(boolean)} callback
 */
chrome.metricsPrivate.getIsCrashReportingEnabled = function(callback) {};


/**
 * Describes the type of metric that is to be collected.
 * @typedef {{
 *    metricName: string,
 *    type: string,
 *    min: number,
 *    max: number,
 *    buckets: number
 * }}
 */
var MetricType;


/**
 * Adds a value to the given metric.
 * @param {MetricType} metricType
 * @param {number} value
 */
chrome.metricsPrivate.recordValue = function(metricType, value) {};


//////////////////////////////////////////////////////////////////////////////
// Externs subset for settingsPrivate API.
// see chromium/src/third_party/closure_compiler/externs/settings_private.js
//////////////////////////////////////////////////////////////////////////////


/** @const {*} */
chrome.settingsPrivate = {};


/**
 * @enum {string}
 */
chrome.settingsPrivate.PrefType = {
  BOOLEAN: 'BOOLEAN',
  NUMBER: 'NUMBER',
  STRING: 'STRING',
  URL: 'URL',
  LIST: 'LIST',
  DICTIONARY: 'DICTIONARY',
};


/**
 * @typedef {{
 *   key: string,
 *   type: !chrome.settingsPrivate.PrefType,
 *   value: *,
 * }}
 */
chrome.settingsPrivate.PrefObject;


/**
 * @param {string} name
 * @param {function(!chrome.settingsPrivate.PrefObject):void} callback
 */
chrome.settingsPrivate.getPref = function(name, callback) {};


/**
 * @const
 */
chrome.settingsPrivate.onPrefsChanged = {};


/**
 * @param {function(!Array<!Object>):void} callback
 */
chrome.settingsPrivate.onPrefsChanged.hasListener = function(callback) {};


/**
 * @param {function(!Array<!Object>):void} callback
 */
chrome.settingsPrivate.onPrefsChanged.addListener = function(callback) {};


/**
 * @param {function(!Array<!Object>):void} callback
 */
chrome.settingsPrivate.onPrefsChanged.removeListener = function(callback) {};


//////////////////////////////////////////////////////////////////////////////
// Externs for Google Calendar v3 API as in
// developers.google.com/google-apps/calendar/v3/reference/events#resource
//////////////////////////////////////////////////////////////////////////////


/** @const */
chrome.cast.calendar = {};



/** @constructor */
chrome.cast.calendar.Calendar = function() {};


/** @type {string} */
chrome.cast.calendar.Calendar.prototype.id;



/** @constructor */
chrome.cast.calendar.Event = function() {};


/** @type {string} */
chrome.cast.calendar.Event.prototype.summary;


/** @type {string} */
chrome.cast.calendar.Event.prototype.hangoutLink;


/**
 * @typedef {{
 *    date: string,
 *    dateTime: string
 * }}
 */
chrome.cast.calendar.Event.prototype.start;


/**
 * @typedef {{
 *    date: string,
 *    dateTime: string
 * }}
 */
chrome.cast.calendar.Event.prototype.end;


//////////////////////////////////////////////////////////////////////////////
// Externs for Google Hangouts v1 API
//////////////////////////////////////////////////////////////////////////////


/** @const */
chrome.cast.hangout = {};


/**
 * @typedef {{
 *   'service': (string|undefined),
 *   'value': (string|undefined)
 * }}
 */
chrome.cast.hangout.ExternalKey;


/**
 * @typedef {{
 *   'hangout_id': (string|undefined),
 *   'participant_id': (string|undefined),
 *   'user_id': (string|undefined),
 *   'display_name': (string|undefined),
 *   'role': (string|undefined),
 *   'client_type': (string|undefined),
 *   'participant_state': (string|undefined),
 *   'joined': (boolean|undefined)
 * }}
 */
chrome.cast.hangout.Participant;


/**
 * @typedef {{
 *   'hangout_id': (string|undefined),
 *   'type': (string|undefined),
 *   'external_key': (chrome.cast.hangout.ExternalKey|undefined),
 *   'company_title': (string|undefined),
 *   'meeting_room_name': (string|undefined),
 *   'meeting_domain': (string|undefined),
 *   "sharing_url": (string|undefined),
 * }}
 */
chrome.cast.hangout.Hangout;



/**

 * @see https://developer.chrome.com/extensions/tabs#event-onUpdated
 * @constructor
 */
function TabChangeInfo() {}


/** @type {string} */
TabChangeInfo.prototype.status;


/** @type {string} */
TabChangeInfo.prototype.url;


/** @type {boolean} */
TabChangeInfo.prototype.pinned;


/** @type {boolean} */
TabChangeInfo.prototype.audible;


/** @type {string} */
TabChangeInfo.prototype.favIconUrl;

//////////////////////////////////////////////////////////////////////////////
// Externs for declarativeWebRequest (not used except for channel checking)
//////////////////////////////////////////////////////////////////////////////


/** @type {Object} */
chrome.declarativeWebRequest;
