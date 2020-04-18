// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.provide('mr.webrtc.AuthReadyMessageData');
goog.provide('mr.webrtc.ChannelType');
goog.provide('mr.webrtc.Message');
goog.provide('mr.webrtc.MessageType');
goog.provide('mr.webrtc.OfferMessageData');

goog.require('mr.mirror.Settings');

goog.scope(function() {


/**
 * The channel types supported by the cloud MRP.
 * @enum {string}
 */
mr.webrtc.ChannelType = {
  WEAVE: 'weave',
  SLARTI: 'slarti',
  MESI: 'mesi'
};


/**
 * The types of messages used by the cloud MRP.
 * @enum {string}
 */
mr.webrtc.MessageType = {
  // TURN messages.
  GET_TURN_CREDENTIALS: 'GET_TURN_CREDENTIALS',  // Request for creds.
  TURN_CREDENTIALS: 'TURN_CREDENTIALS',          // Creds received.

  // Signaling messages.
  OFFER: 'OFFER',                // Peer connection offer.
  ANSWER: 'ANSWER',              // Peer connection answer.
  KNOCK_ANSWER: 'KNOCK_ANSWER',  // Knocking peer connection answer.
  STOP: 'STOP',                  // Stop the session.

  // Event messages.
  SESSION_START_SUCCESS: 'SESSION_START_SUCCESS',  // Start success event.
  SESSION_FAILURE: 'SESSION_FAILURE',              // Start failure event.
  SESSION_END: 'SESSION_END',                      // Session ended event.

  WEB_RTC_STATS: '__webrtc_stats__',  // WebRTC stats message.

  // Hangout session control messages.
  REFRESH_AUTH: 'REFRESH_AUTH',  // Request to refresh the auth token.
  // Message data for AUTH_READY messages will be an instance of
  // mr.webwrtc.AuthReadyMessageData.
  AUTH_READY: 'AUTH_READY',  // Response that auth token was updated.

  // Route details control messages.
  MUTE: 'MUTE',                            // Request to mute audio.
  LOCAL_PRESENT: 'LOCAL_PRESENT',          // Request to disable conf mode.
  ROUTE_STATUS_REQUEST: 'STATUS_REQUEST',  // Request for route status update.
  ROUTE_STATUS_RESPONSE:
      'STATUS_RESPONSE',  // Response to route details status.

  // Hangout issues.
  HANGOUT_INVALID: 'HANGOUT_INVALID',    // Hangout name could not be resolved.
  HANGOUT_INACTIVE: 'HANGOUT_INACTIVE',  // Not enough participants in Hangout.

  // Application messages sent through Presentation API.
  PRESENTATION_CONNECTION_MESSAGE: 'PRESENTATION_CONNECTION_MESSAGE',
};


/**
 * Cloud message object used for internal communication.
 */
mr.webrtc.Message = class {
  /**
   * @param {mr.webrtc.MessageType} type
   * @param {!Object|undefined=} opt_data
   */
  constructor(type, opt_data) {
    /**
     * @type {mr.webrtc.MessageType}
     * @export
     */
    this.type = type;
    /**
     * @type {!Object|undefined}
     * @export
     */
    this.data = opt_data;
  }

  /**
   * Returns the message for the provided JSON string.
   * @param {string} messageStr
   * @return {!mr.webrtc.Message}
   */
  static fromString(messageStr) {
    const messageJson = JSON.parse(messageStr);
    if (!messageJson['type']) {
      throw Error('Invalid message');
    }
    return new Message(
        /** @type {mr.webrtc.MessageType} */ (messageJson['type']),
        /** @type {!Object|undefined} */ (messageJson['data']));
  }

  /**
   * Constructs an AUTH_READY message.
   * @param {!mr.webrtc.AuthReadyMessageData} data
   * @return {!mr.webrtc.Message}
   */
  static authReady(data) {
    return new Message(mr.webrtc.MessageType.AUTH_READY, data);
  }

  /**
   * Workaround for broken handling of ES6 classes in Jasmine.
   * @return {string}
   */
  jasmineToString() {
    return '[mr.webrtc.Message instance]';
  }
};

const Message = mr.webrtc.Message;


/**
 * The data for an offer message. Setting the presentation url in the WebRTC
 * offer message indicates to the receiver that the session is a presentation
 * session.
 */
mr.webrtc.OfferMessageData = class {
  /**
   * @param {RTCSessionDescription} description
   * @param {mr.mirror.Settings=} opt_settings
   * @param {MediaConstraints=} opt_mediaConstraints
   * @param {string=} opt_presentationUrl
   * @param {string=} opt_presentationId
   */
  constructor(
      description, opt_settings, opt_mediaConstraints, opt_presentationUrl,
      opt_presentationId) {
    /**
     * @type {RTCSessionDescription}
     * @export
     */
    this.description = description;

    /**
     * @type {?mr.mirror.Settings}
     * @export
     */
    this.settings = opt_settings || null;

    /**
     * @type {?MediaConstraints}
     * @export
     */
    this.mediaConstraints = opt_mediaConstraints || null;

    /**
     * @type {?string}
     * @export
     */
    this.presentationUrl = opt_presentationUrl || null;

    /**
     * @type {?string}
     * @export
     */
    this.presentationId = opt_presentationId || null;
  }
};


/**
 * Data associated with an AUTH_READY message.
 *
 * Fields:
 *
 * - isMeeting: True for Thor meetings, false for Hangouts.
 *
 * - hangoutId: The public ID of the Hangout/meeting.
 *
 * - resolvedId: The resolved (internal) ID of the Hangout/meeting.  May be ''.
 *
 * - conferenceMode: If defined, used to override the setting of the
 *   isConferenceMode_ field of HangoutSession.
 *
 * @typedef {{
 *   isMeeting: boolean,
 *   hangoutId: string,
 *   resolvedId: string,
 *   conferenceMode: (boolean|undefined)
 * }}
 */
mr.webrtc.AuthReadyMessageData;

});  // goog.scope
