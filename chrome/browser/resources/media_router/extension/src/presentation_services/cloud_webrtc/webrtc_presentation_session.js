// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Implementation of PresentationSession that uses the WebRTC
 *     PeerConnection.
 */
goog.provide('mr.presentation.webrtc.CloudWebRtcSession');

goog.require('mr.MessagePortService');
goog.require('mr.PromiseResolver');
goog.require('mr.presentation.Session');
goog.require('mr.webrtc.Message');
goog.require('mr.webrtc.MessageType');
goog.require('mr.webrtc.OfferMessageData');
goog.require('mr.webrtc.PeerConnection');

goog.scope(function() {




/**
 * Constructs a new WebRTC presentation session.
 * @implements {mr.presentation.Session}
 */
mr.presentation.webrtc.CloudWebRtcSession = class {
  /**
   * @param {!mr.Route} route
   * @param {!string} sourceUrn
   */
  constructor(route, sourceUrn) {
    /** @type {!mr.Route} */
    this.route = route;

    /** @type {!string} */
    this.sourceUrn = sourceUrn;

    /** @private {!mr.PromiseResolver<!mr.webrtc.PeerConnection>} */
    this.peerConnectionResolver_ = new mr.PromiseResolver();

    /** @private {!Promise<!mr.webrtc.PeerConnection>} */
    this.peerConnection_ = this.peerConnectionResolver_.promise;

    /** @private {!mr.MessagePort} */
    this.messagePort_ =
        mr.MessagePortService.getService().getInternalMessenger(route.id);

    /** @private {!mr.PromiseResolver<!mr.Route>} */
    this.startResolver_ = new mr.PromiseResolver();

    /** @private {boolean} */
    this.started_ = false;

    this.setUpMessagePort_();
    this.setUpPeerConnection_();

    // Send a request for TURN credentials, then expect a response message with
    // type TURN_CREDENTIALS.
    this.sendMessageToMrp_(
        new mr.webrtc.Message(mr.webrtc.MessageType.GET_TURN_CREDENTIALS));
  }

  /**
   * @override
   */
  start() {
    return this.peerConnection_.then(pc => {
      if (pc.isStarted()) {
        return Promise.reject(Error('Presentation already started'));
      }

      pc.start();
      return this.startResolver_.promise;
    });
  }

  /**
   * @override
   */
  stop() {
    this.started_ = false;
    return this.peerConnection_.then(pc => {
      pc.stop();
      this.peerConnection_ =
          Promise.reject(Error('Peer connection has already been stopped'));
    });
  }

  /**
   * Sets up the message port to receive incoming messages.
   * @private
   */
  setUpMessagePort_() {
    this.messagePort_.onMessage = message => {
      if (!message.type) {
        // Wrap message and send it along as a presentation message.
        this.peerConnection_.then(pc => {
          pc.sendDataChannelMessage({
            type: mr.webrtc.MessageType.PRESENTATION_CONNECTION_MESSAGE,
            data: message
          });
        });
        return;
      }
      switch (message.type) {
        case mr.webrtc.MessageType.TURN_CREDENTIALS:
          // Response to a GET_TURN_CREDENTIALS message. This causes the
          // PeerConnection to be created.
          this.peerConnectionResolver_.resolve(new mr.webrtc.PeerConnection(
              this.route.id,
              /** @type {!Array<!mr.webrtc.TurnCredential>} */
              (message.data['credentials'])));
          break;
        case mr.webrtc.MessageType.ANSWER:
          this.peerConnection_.then(pc => {
            pc.setRemoteDescription(message.data);
          });
          break;
        case mr.webrtc.MessageType.STOP:
          this.startResolver_.reject('Stop signal received');
          this.stop();
          break;
        default:
          throw Error('Unknown message type: ' + message.type);
      }
    };
  }

  /**
   * Sets up the peer connection (with callbacks).
   * @private
   */
  setUpPeerConnection_() {
    this.peerConnection_.then(pc => {
      // Pass the description up the MessagePort.
      pc.setOnOfferDescriptionReady(description => {
        const offerData = new mr.webrtc.OfferMessageData(
            description,
            /* opt_settings_ */ null,
            /* opt_mediaConstraints */ null, this.sourceUrn, this.route.id);
        const message =
            new mr.webrtc.Message(mr.webrtc.MessageType.OFFER, offerData);
        this.sendMessageToMrp_(message);
      });
      // Pass along the data channel message up the MessagePort.
      pc.setOnDataChannelMessage(message => {
        // Check if message is a STOP message and calls stop() before sending it
        // through the message port to MRP.
        const webRtcMessage = mr.webrtc.Message.fromString(message);
        if (webRtcMessage.type == mr.webrtc.MessageType.STOP) {
          this.stop();
        }
        this.sendMessageToMrp_(webRtcMessage);
      });

      // Send the connection success/closed/failed events up the MessagePort,
      // and
      // also resolve or reject the start() promise.
      pc.setOnConnectionSuccess(event => {
        this.started_ = true;
        this.sendMessageToMrp_(
            new mr.webrtc.Message(mr.webrtc.MessageType.SESSION_START_SUCCESS));
        this.startResolver_.resolve(this.route);
      });
      pc.setOnConnectionClosed(event => {
        this.sendMessageToMrp_(
            new mr.webrtc.Message(mr.webrtc.MessageType.SESSION_END));
      });
      pc.setOnConnectionFailure(error => {
        // If we haven't started yet, reject the start promise.
        if (!this.started_) {
          this.startResolver_.reject(error);
        }
        this.sendMessageToMrp_(
            new mr.webrtc.Message(mr.webrtc.MessageType.SESSION_FAILURE));
      });
    });
  }

  /**
   * Sends the provided message to the MRP via the message port.
   * @param {!Object} message
   * @private
   */
  sendMessageToMrp_(message) {
    this.messagePort_.sendMessage(message);
  }
};

});  // goog.scope
