// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.provide('mr.webrtc.PeerConnection');
goog.provide('mr.webrtc.TurnCredential');

goog.require('mr.Assertions');
goog.require('mr.Logger');
goog.require('mr.PromiseResolver');
goog.require('mr.webrtc.PeerConnectionAnalytics');

goog.scope(function() {


/**
 * Type of credentials returned by the TURN credential service.  Can be used
 * as-is in the 'iceConnection' property of the configuration passed to the
 * webkitRTCPeerConnection() constructor.
 *
 * @typedef {{
 *   username: string,
 *   credential: string,
 *   url: string
 * }}
 */
mr.webrtc.TurnCredential;


/**
 * Creates a new PeerConnection.
 */
mr.webrtc.PeerConnection = class {
  /**
   * @param {string} routeId The route ID for the route to open this
   *     PeerConnection. Used as the data channel ID.
   * @param {!Array<!mr.webrtc.TurnCredential>} turnCreds
   *     TURN server credentials.
   */
  constructor(routeId, turnCreds) {
    mr.Assertions.assert(
        webkitRTCPeerConnection !== undefined,
        'webkitRTCPeerConnection is not available.  Do you need to set flags?');

    /**
     * Logger instance.
     * @private {mr.Logger}
     */
    this.logger_ = mr.Logger.getInstance('cv2.PeerConnection');

    /**
     * The media constraints to assign to the PeerConnection.
     * @type {!MediaConstraints | undefined}
     * @private
     */
    this.mediaConstraints_ = PeerConnection.MEDIA_CONSTRAINTS;

    /**
     * WebRTC PeerConnection wrapped by this class.
     * @private {webkitRTCPeerConnection}
     */
    this.peerConnection_ = this.createPeerConnection_(turnCreds);

    /**
     * WebRTC data channel (if it should be enabled).
     * @type {!RTCDataChannel}
     * @private
     */
    this.dataChannel_ = this.createDataChannel_(routeId);



    /**
     * Whether we will try an ICE restart when we are in a disconnected state.
     * @private {boolean}
     */
    this.enableIceRestart_ = true;



    /**
     * Resolves when the Web RTC session description is received.
     * @private {!mr.PromiseResolver<RTCSessionDescription>}
     */
    this.sessionDescriptionResolver_ = new mr.PromiseResolver();

    /**
     * True if sessionDescriptionResolver_ has been resolved.
     * @private {boolean}
     */
    this.sessionDescriptionResolved_ = false;

    /**
     * The number of ICE candidates that have been received so far.
     * @private {number}
     */
    this.numIceCandidatesReceived_ = 0;

    /**
     * ID of the timer used to abort ICE candidate gathering.
     * @private {?number}
     */
    this.iceCandidateGatheringTimerId_ = null;

    /**
     * The time when the offer is created.
     * @private {number}
     */
    this.offerCreationTime_ = 0;

    /**
     * The time when the most recent ICE candidate was received.
     * @private {number}
     */
    this.lastIceCandidateTime_ = 0;

    /**
     * True if the PeerConnection has been started.
     * @private {boolean}
     */
    this.started_ = false;

    /**
     * Callback invoked when a PeerConnection has connection issues.
     * @private {function()}
     */
    this.onConnectionStale_ = () => {};

    /**
     * Callback invoked when the offer description is ready.
     * @private {function(RTCSessionDescription)}
     */
    this.onOfferDescription_ = () => {};

    /**
     * Callback invoked when the connection is successfully made.
     * @private {function(mr.webrtc.PeerConnection.Event)}
     */
    this.onConnectionSuccess_ = () => {};

    /**
     * Callback invoked when the connection fails.
     * @private {function(mr.webrtc.PeerConnection.Event)}
     */
    this.onConnectionFailure_ = () => {};

    /**
     * Callback invoked when the connection is closed.
     * @type {function(mr.webrtc.PeerConnection.Event)}
     * @private
     */
    this.onConnectionClosed_ = () => {};
  }

  /**
   * @param {function()} staleFn The callback.
   */
  setOnConnectionStale(staleFn) {
    this.onConnectionStale_ = staleFn;
  }

  /**
   * @param {function(mr.webrtc.PeerConnection.Event)} successFn
   */
  setOnConnectionSuccess(successFn) {
    this.onConnectionSuccess_ = successFn;
  }

  /**
   * @param {function(mr.webrtc.PeerConnection.Event)} failureFn
   */
  setOnConnectionFailure(failureFn) {
    this.onConnectionFailure_ = failureFn;
  }

  /**
   * @param {function(mr.webrtc.PeerConnection.Event)} closedFn
   */
  setOnConnectionClosed(closedFn) {
    this.onConnectionClosed_ = closedFn;
  }

  /**
   * @param {function(RTCSessionDescription)} onOfferFn
   */
  setOnOfferDescriptionReady(onOfferFn) {
    this.onOfferDescription_ = onOfferFn;
  }

  /**
   * @param {function(string)} onMessageFn
   */
  setOnDataChannelMessage(onMessageFn) {
    this.dataChannel_.onmessage = function(event) {
      onMessageFn(event.data);
    };
  }

  /**
   * @param {boolean} shouldEnable Whether we should enable ICE restart.
   */
  enableIceRestart(shouldEnable) {
    this.enableIceRestart_ = shouldEnable;
  }

  /**
   * @return {boolean} true if the PeerConnection has been started.
   */
  isStarted() {
    return this.started_;
  }

  /**
   * Returns the configuration data for the WebRTC PeerConnection.
   * @return {!RTCConfiguration} The configuration.
   * @param {!Array<!mr.webrtc.TurnCredential>} turnCreds
   *     TURN server credentials.
   * @private
   * @suppress {invalidCasts} invalid cast - must be a subtype or supertype
   * from: {}
   * to  : (!RTCConfiguration)
   */
  getPeerConnectionConfig_(turnCreds) {
    const server = {};
    server['url'] = 'stun:stun.l.google.com:19302';
    const config = {};
    config['iceServers'] = [server].concat(turnCreds);
    return /** @type {!RTCConfiguration} */ (config);
  }

  /**
   * Creates the WebRTC PeerConnection object.
   * @return {webkitRTCPeerConnection} The new PC.
   * @param {!Array<!mr.webrtc.TurnCredential>} turnCreds
   *     TURN server credentials.
   * @private
   */
  createPeerConnection_(turnCreds) {
    const config = this.getPeerConnectionConfig_(turnCreds);
    const peerConnection = new webkitRTCPeerConnection(config);
    peerConnection.onicecandidate = this.onIceCandidate_.bind(this);
    peerConnection.onicegatheringstatechange =
        this.onIceGatheringStateChange_.bind(this);
    peerConnection.oniceconnectionstatechange =
        this.onIceConnectionStateChange_.bind(this);
    this.logger_.info(
        () => 'Created webkitRTCPeerConnnection with config: ' +
            JSON.stringify(config));
    return peerConnection;
  }

  /**
   * Creates a data channel. Must be called in the initiator before the SDP is
   * created.
   * @param {string} channelId
   * @return {!RTCDataChannel}
   * @private
   */
  createDataChannel_(channelId) {
    const dataChannel =
        this.peerConnection_.createDataChannel(channelId, {'reliable': false});
    return dataChannel;
  }

  /**
   * Sends the provided message via the data channel.
   * @param {!Object|string} message The message to send.
   */
  sendDataChannelMessage(message) {
    if (typeof message == 'string') {
      this.dataChannel_.send(message);
    } else {
      this.dataChannel_.send(JSON.stringify(message));
    }
  }

  /**
   * Starts the PeerConnection with any added streams.
   */
  start() {
    if (!this.started_) {
      this.started_ = true;
      // Caller initiates offer to peer.
      this.createOffer_();
    }
  }

  /**
   * Stops this PeerConnection.
   */
  stop() {
    this.logger_.info('Stopping peer connection...');
    if (this.started_) {
      this.started_ = false;
      if (this.peerConnection_.signalingState != 'closed') {
        this.peerConnection_.close();
      }
    }
    this.peerConnection_ = null;
  }

  /**
   * Adds a stream to the PeerConnection.
   * @param {!MediaStream} stream The media stream to add.
   */
  addStream(stream) {
    this.peerConnection_.addStream(stream);
  }

  /**
   * Removes a stream from the PeerConnection.
   * @param {!MediaStream} stream The media stream to remove.
   */
  removeStream(stream) {
    if (this.started_) {
      this.peerConnection_.removeStream(stream);
    }
  }

  /**
   * Initiates a call to the peer.
   * @private
   */
  createOffer_() {
    this.logger_.info('Sending offer to peer.');
    this.offerCreationTime_ = Date.now();

    this.peerConnection_.createOffer(
        this.setLocalDescription_.bind(this), error => {
          this.logger_.warning('Error creating offer.', error);
        }, this.mediaConstraints_);
    this.getSessionDescription().then(sessionDescription => {
      this.onOfferDescription_(sessionDescription);
    });
  }

  /**
   * Sets the local description for the session.
   * @param {!RTCSessionDescription} sessionDescription The offer.
   * @private
   */
  setLocalDescription_(sessionDescription) {

    this.logger_.info(
        () =>
            'Setting local description: ' + JSON.stringify(sessionDescription));
    this.peerConnection_.setLocalDescription(
        sessionDescription,
        () => {
          this.logger_.info('Local description set successfully');
        },
        error => {
          this.logger_.warning('Error setting local description.', error);
        });
    // Cloud connections only send messages when ICE gathering is complete.
    // This is done in createOffer_ for cloud connections instead.
  }

  /**
   * There's currently a WebRTC "bug" which means we can't JSON.stringify an
   * RTCSessionDescription. See http://b/19817649. So for now, we'll just put it
   * in our own object.

   * @param {RTCSessionDescription} description
   * @return {RTCSessionDescription}
   * @private
   */
  formDescriptionMessage_(description) {
    return /** @type {RTCSessionDescription} */ (
        {'type': description.type, 'sdp': description.sdp});
  }

  /**
   * Returns the session offer description (if this is the sender) or answer
   * description (if this is the receiver).
   * Note that this description contains all of the ICE candidates as well.
   * @return {!Promise<RTCSessionDescription>}
   */
  getSessionDescription() {
    return this.sessionDescriptionResolver_.promise;
  }

  /**
   * Sets the remote description on the peer connection.
   * @param {!RTCSessionDescription} sessionDescription
   */
  setRemoteDescription(sessionDescription) {
    this.logger_.fine(() => '<===: ' + JSON.stringify(sessionDescription));
    const description = new RTCSessionDescription(sessionDescription);
    this.logger_.info(
        () => 'Setting remote description: ' + JSON.stringify(description));
    // We received an answer! Just set the description.
    this.peerConnection_.setRemoteDescription(
        description,
        () => {
          this.logger_.info('Remote description set successfully.');
        },
        error => {
          this.logger_.warning('Error setting remote description.', error);
        });
  }

  /**
   * Resolves the session description once all ice candidates have been
   * received.
   * @param {RTCPeerConnectionIceEvent} event The ICE candidate event.
   * @private
   */
  onIceCandidate_(event) {
    if (event.candidate) {
      this.numIceCandidatesReceived_++;
      this.lastIceCandidateTime_ = Date.now();
      if (this.numIceCandidatesReceived_ == 1) {
        // This is the first ICE candidate.  Set a timer to abort gathering
        // candidates.  This is needed because sometimes the end of ICE
        // candidate
        // gathering is not detected right away even though all candidates have
        // been gathered.
        mr.Assertions.assert(this.iceCandidateGatheringTimerId_ == null);
        this.iceCandidateGatheringTimerId_ = setTimeout(() => {
          this.logger_.info('ICE candidate gathering timed out.');
          this.iceCandidateGatheringTimerId_ = null;
          this.resolveSessionDescription_();
        }, PeerConnection.ICE_CANDIDATE_GATHERING_TIMEOUT_MS_);
      } else if (this.sessionDescriptionResolved_) {
        // This branch runs when additional ICE candidates are reported after
        // the timeout above fires.
        this.logger_.warning(
            'Received ICE candidate after resolving session description.');
      }
    } else {
      this.logger_.info('End of ICE candidates.');
      mr.webrtc.PeerConnectionAnalytics
          .recordIceCandidateGatheringReportedDuration(
              Date.now() - this.offerCreationTime_);

      // This is a no-op if the timout above has already fired.
      this.resolveSessionDescription_();

      // Record the true duration of candidate gathering based on the time the
      // last candidate was reported.  Don't record anything if no candidates
      // were
      // found, because the computed duration will be invalid.
      if (this.numIceCandidatesReceived_ > 0) {
        mr.webrtc.PeerConnectionAnalytics
            .recordIceCandidateGatheringRealDuration(
                this.lastIceCandidateTime_ - this.offerCreationTime_);
      }
    }
  }

  /**
   * Called when ICE gathering state changes. Tracks when the candidates are
   * complete.
   * @private
   */
  onIceGatheringStateChange_() {
    // This method never appears to be called.
    const state = this.peerConnection_.iceGatheringState;
    if (state == 'completed') {
      this.resolveSessionDescription_();
    }
  }

  /**
   * Resolves the session description promise.  Should be called after all ICE
   * candidates have been received.
   * @private
   */
  resolveSessionDescription_() {
    clearTimeout(this.iceCandidateGatheringTimerId_);
    this.iceCandidateGatheringTimerId_ = null;
    if (!this.sessionDescriptionResolved_) {
      this.logger_.info(
          'Resolving sesion description after gathering ' +
          this.numIceCandidatesReceived_ + ' ICE candidates.');
      this.sessionDescriptionResolver_.resolve(
          this.formDescriptionMessage_(this.peerConnection_.localDescription));
      this.sessionDescriptionResolved_ = true;
    }
  }

  /**
   * Handles ICE connection state changes. Tries to restart PeerConnection when
   * we
   * are in a disconnected state by creating a new offer with the IceRestart
   * constraint.
   * @param {Event} event
   * @private
   */
  onIceConnectionStateChange_(event) {
    if (!this.peerConnection_) return;

    const state = this.peerConnection_.iceConnectionState;
    this.logger_.info('New ICE connection state: ' + state + '.');
    if (state == 'connected') {
      this.onConnectionSuccess_(PeerConnection.Event.ICE_CONNECTED);
    } else if (state == 'completed') {
      this.onConnectionSuccess_(PeerConnection.Event.ICE_COMPLETED);
    } else if (state == 'failed') {
      this.logger_.warning(
          () => 'Ice connection failed: ' + JSON.stringify(event));
      this.onConnectionFailure_(PeerConnection.Event.ICE_FAILED);
    } else if (state == 'closed') {
      this.onConnectionClosed_(PeerConnection.Event.ICE_CLOSED);
    } else if (state == 'disconnected') {
      this.logger_.warning('Ice connection state is bad.');
      if (this.enableIceRestart_ && this.isStarted()) {
        this.logger_.info('Restarting ICE.');
        this.peerConnection_.createOffer(
            this.setLocalDescription_.bind(this), error => {
              this.logger_.warning('Error creating new offer.', error);
            }, PeerConnection.ICE_RESTART_MEDIA_CONSTRAINTS);
      } else {
        this.onConnectionStale_();
      }
    }
  }
};

const PeerConnection = mr.webrtc.PeerConnection;


/**
 * Default media constraints for tab capture.
 * @const {!MediaConstraints}
 */
PeerConnection.MEDIA_CONSTRAINTS = {
  'mandatory': {'OfferToReceiveAudio': true, 'OfferToReceiveVideo': true}
};


/**
 * Default media constraints during ICE restarts.
 * @const {!MediaConstraints}
 */
PeerConnection.ICE_RESTART_MEDIA_CONSTRAINTS = {
  'mandatory': {
    'IceRestart': true,
    'OfferToReceiveAudio': true,
    'OfferToReceiveVideo': true
  }
};


/**
 * PeerConnection event types.
 * @enum {string}
 */
PeerConnection.Event = {
  // events that have a corresponding on on<Event> callback.
  ADD_STREAM: 'addstream',
  REMOVE_STREAM: 'removestream',
  ICE_CANDIDATE: 'icecandidate',
  // events that do not have a corresponding on on<Event> callback.
  ICE_CONNECTED: 'iceconnected',
  ICE_COMPLETED: 'icecompleted',
  ICE_FAILED: 'icefailed',
  ICE_CLOSED: 'iceclosed'
};


/**
 * The maximum time to wait, in ms, for all ICE candidates to be gathered.
 * Timing starts when the first ICE candidate is seen.
 * @private @const
 */
PeerConnection.ICE_CANDIDATE_GATHERING_TIMEOUT_MS_ = 5 * 1000;

});  // goog.scope
