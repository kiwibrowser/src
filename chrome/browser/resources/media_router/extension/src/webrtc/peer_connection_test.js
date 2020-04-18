// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Tests for peer_connection.
 */
goog.setTestOnly('peer_connection_test');

goog.require('mr.webrtc.PeerConnection');

describe('mr.webrtc.PeerConnection', function() {
  let peerConnection;
  let mockWebkitPeerConnection, mockDataChannel;
  let mockOnConnectionSuccess, mockOnConnectionStale;
  let mockOnConnectionClosed, mockOnConnectionFailure;
  let mockOnDescriptionFn, mockOnDataChannelMessage;
  let mockClock;

  const DATA_CHANNEL_NAME = 'TEST_DATA_CHANNEL';

  beforeEach(function() {
    mockOnDescriptionFn = jasmine.createSpy('mockOnDescriptionFn');
    mockOnConnectionSuccess = jasmine.createSpy('mockOnConnectionSuccess');
    mockOnConnectionClosed = jasmine.createSpy('mockOnConnectionClosed');
    mockOnConnectionFailure = jasmine.createSpy('mockOnConnectionFailure');
    mockOnConnectionStale = jasmine.createSpy('mockOnConnectionStale');
    mockOnDataChannelMessage = jasmine.createSpy('mockOnDataChannelMessage');
    // There seems to no longer be a prototype exposed for
    // webkitRTCPeerConnection as of cr43. Likely duplicate of b/19817649 (and
    // related). So create a dumb Jasmine mock object with all methods we care
    // about.
    mockWebkitPeerConnection = jasmine.createSpyObj('peerConnection', [
      'close', 'createOffer', 'createAnswer', 'createDataChannel', 'addStream',
      'setLocalDescription', 'setRemoteDescription', 'addIceCandidate',
      'removeStream', 'oniceconnected', 'onicecompleted', 'onicefailed',
      'onicecandidate', 'oniceconnectionstatechange'
    ]);
    mockDataChannel =
        jasmine.createSpyObj('dataChannel', ['onmessage', 'send']);
    mockWebkitPeerConnection.createDataChannel.and.returnValue(mockDataChannel);
    webkitRTCPeerConnection = function(config) {
      return mockWebkitPeerConnection;
    };
    peerConnection = new mr.webrtc.PeerConnection(DATA_CHANNEL_NAME);
    peerConnection.setOnConnectionSuccess(mockOnConnectionSuccess);
    peerConnection.setOnConnectionClosed(mockOnConnectionClosed);
    peerConnection.setOnConnectionFailure(mockOnConnectionFailure);
    peerConnection.setOnConnectionStale(mockOnConnectionStale);
    peerConnection.setOnOfferDescriptionReady(mockOnDescriptionFn);
  });

  it('constructor creates webkit peer connection with data channel, ' +
         'does not start',
     function() {
       expect(peerConnection.isStarted()).toEqual(false);
       expect(peerConnection.peerConnection_).toEqual(mockWebkitPeerConnection);
       expect(peerConnection.dataChannel_).toEqual(mockDataChannel);
       expect(mockWebkitPeerConnection.createDataChannel)
           .toHaveBeenCalledWith(DATA_CHANNEL_NAME, {'reliable': false});
     });

  it('data channel onmessage calls callback', function() {
    peerConnection.setOnDataChannelMessage(mockOnDataChannelMessage);
    const event = {'data': 'DATA!!!'};
    mockDataChannel.onmessage(event);
    expect(mockOnDataChannelMessage).toHaveBeenCalledWith(event.data);
  });

  it('sendDataChannelMessage sends message via data channel', function() {
    // String message.
    const stringMessage = 'String message!';
    peerConnection.sendDataChannelMessage(stringMessage);
    expect(mockDataChannel.send).toHaveBeenCalledWith(stringMessage);

    // Object message.
    const objMessage = {'obj': 'message'};
    peerConnection.sendDataChannelMessage(objMessage);
    expect(mockDataChannel.send)
        .toHaveBeenCalledWith(JSON.stringify(objMessage));
  });

  it('start creates offer, sets local description and calls on description ' +
         'callback message when ready',
     function(done) {
       peerConnection.start();

       expect(peerConnection.isStarted()).toEqual(true);
       expect(mockWebkitPeerConnection.createOffer).toHaveBeenCalled();
       const createOfferArgs =
           mockWebkitPeerConnection.createOffer.calls.mostRecent().args;
       expect(createOfferArgs[2])
           .toEqual(mr.webrtc.PeerConnection.MEDIA_CONSTRAINTS);

       // Now call the local description callback (first arg)
       const description = {'sdp': 'SDP!', 'type': 'TYPE!'};
       createOfferArgs[0](description);
       const localDescriptionArgs =
           mockWebkitPeerConnection.setLocalDescription.calls.mostRecent().args;
       expect(localDescriptionArgs[0]).toEqual(description);

       // Now trigger the message sending by triggering ICE complete.
       const webkitLocalDescription = {
         'sdp': 'Local SDP with ICE candidates!',
         'type': 'Local Type!'
       };
       mockWebkitPeerConnection.localDescription = webkitLocalDescription;
       peerConnection.onIceCandidate_({
         'candidate': null  // empty candidate signifies that it's done.
       });
       mockOnDescriptionFn.and.callFake(arg => {
         expect(arg).toEqual(webkitLocalDescription);
         done();
       });
     });

  it('stop closes the webkit peer connection', function() {
    peerConnection.started_ = true;
    peerConnection.stop();

    expect(peerConnection.isStarted()).toEqual(false);
    expect(mockWebkitPeerConnection.close).toHaveBeenCalled();
  });

  it('addStream adds the stream to the webkit peer connection', function() {
    const stream = {'fake': 'media stream'};
    peerConnection.addStream(stream);

    expect(mockWebkitPeerConnection.addStream).toHaveBeenCalledWith(stream);
  });

  it('removeStream removes the stream from the webkit peer connection',
     function() {
       const stream = {'fake': 'media stream'};
       peerConnection.started_ = true;
       peerConnection.removeStream(stream);

       expect(mockWebkitPeerConnection.removeStream)
           .toHaveBeenCalledWith(stream);
     });

  it('setRemoteDescription sets remote description', function() {
    const description = {'sdp': 'SDP!', 'type': 'TYPE!'};
    const mockRtcSessionDescription = {'sdp': 'RTC SDP!', 'type': 'RTC TYPE!'};
    spyOn(window, 'RTCSessionDescription')
        .and.returnValue(mockRtcSessionDescription);

    peerConnection.setRemoteDescription(description);

    const args =
        mockWebkitPeerConnection.setRemoteDescription.calls.mostRecent().args;
    expect(args[0]).toEqual(mockRtcSessionDescription);
  });

  it('onIceGatheringStateChange_ resolves the session description', done => {
    const description = {'sdp': 'SDP!', 'type': 'TYPE!'};
    mockWebkitPeerConnection.iceGatheringState = 'completed';
    mockWebkitPeerConnection.localDescription = description;
    peerConnection.onIceGatheringStateChange_();

    peerConnection.sessionDescriptionResolver_.promise.then(value => {
      expect(value).toEqual(description);
      done();
    });
  });

  it('onIceConnectionStateChange_ calls success callback when connected',
     function() {
       mockWebkitPeerConnection.iceConnectionState = 'connected';
       peerConnection.onIceConnectionStateChange_({});
       expect(mockOnConnectionSuccess)
           .toHaveBeenCalledWith(mr.webrtc.PeerConnection.Event.ICE_CONNECTED);

       mockOnConnectionSuccess.calls.reset();
       mockWebkitPeerConnection.iceConnectionState = 'completed';
       peerConnection.onIceConnectionStateChange_({});
       expect(mockOnConnectionSuccess)
           .toHaveBeenCalledWith(mr.webrtc.PeerConnection.Event.ICE_COMPLETED);
     });

  it('onConnectionStateChange_ calls closed callback when connection closes',
     function() {
       mockWebkitPeerConnection.iceConnectionState = 'closed';
       peerConnection.onIceConnectionStateChange_({});
       expect(mockOnConnectionClosed)
           .toHaveBeenCalledWith(mr.webrtc.PeerConnection.Event.ICE_CLOSED);
     });

  it('onConnectionStateChange_ calls failure callback when connection fails',
     function() {
       mockWebkitPeerConnection.iceConnectionState = 'failed';
       peerConnection.onIceConnectionStateChange_({});
       expect(mockOnConnectionFailure)
           .toHaveBeenCalledWith(mr.webrtc.PeerConnection.Event.ICE_FAILED);
     });

  it('onIceConnectionStateChange_ tries to re-connect when disconnected',
     function() {
       peerConnection.started_ = true;
       peerConnection.enableIceRestart_ = true;

       mockWebkitPeerConnection.iceConnectionState = 'disconnected';
       peerConnection.onIceConnectionStateChange_({});

       const args =
           mockWebkitPeerConnection.createOffer.calls.mostRecent().args;
       expect(args[2]).toEqual(
           mr.webrtc.PeerConnection.ICE_RESTART_MEDIA_CONSTRAINTS);

       // Instead, try when enableIceRestart is false.
       peerConnection.enableIceRestart_ = false;
       peerConnection.onIceConnectionStateChange_({});
       expect(mockOnConnectionStale).toHaveBeenCalled();
     });
});
