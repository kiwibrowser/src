/**
 * Copyright 2017 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// Public interface to tests. These are expected to be called with
// ExecuteJavascript invocations from the browser tests and will return answers
// through the DOM automation controller.

/**
 * Adds |count| streams to the peer connection, with one audio and one video
 * track per stream.
 *
 * Returns "ok-streams-created-and-added" on success.
 */
function createAndAddStreams(count) {
  if (count > 0) {
    navigator.getUserMedia({ audio: true, video: true },
        function(stream) {
          peerConnection_().addStream(stream);
          createAndAddStreams(count - 1);
        },
        function(error) {
          throw failTest('getUserMedia failed: ' + error);
        });
  } else {
    returnToTest('ok-streams-created-and-added');
  }
}

/**
 * Verifies that the peer connection's getSenders() returns one sender per local
 * track, that there are no duplicates and that object identity is preserved.
 *
 * Returns "ok-senders-verified" on success.
 */
function verifyRtpSenders(expectedNumTracks = null) {
  if (expectedNumTracks != null &&
      peerConnection_().getSenders().length != expectedNumTracks) {
    throw failTest('getSenders().length != expectedNumTracks');
  }
  if (!arrayEquals_(peerConnection_().getSenders(),
                    peerConnection_().getSenders())) {
    throw failTest('One getSenders() call is not equal to the next.');
  }

  let senders = new Set();
  let senderTracks = new Set();
  peerConnection_().getSenders().forEach(function(sender) {
    if (sender == null)
      throw failTest('sender is null or undefined.');
    if (sender.track == null)
      throw failTest('sender.track is null or undefined.');
    senders.add(sender);
    senderTracks.add(sender.track);
  });
  if (senderTracks.size != senders.size)
    throw failTest('senderTracks.size != senders.size');

  returnToTest('ok-senders-verified');
}

/**
 * Verifies that the peer connection's getReceivers() returns one receiver per
 * remote track, that there are no duplicates and that object identity is
 * preserved.
 *
 * Returns "ok-receivers-verified" on success.
 */
function verifyRtpReceivers(expectedNumTracks = null) {
  if (peerConnection_().getReceivers() == null)
    throw failTest('getReceivers() returns null or undefined.');
  if (expectedNumTracks != null &&
      peerConnection_().getReceivers().length != expectedNumTracks) {
    throw failTest('getReceivers().length != expectedNumTracks');
  }
  if (!arrayEquals_(peerConnection_().getReceivers(),
                    peerConnection_().getReceivers())) {
    throw failTest('One getReceivers() call is not equal to the next.');
  }

  let receivers = new Set();
  let receiverTracks = new Set();
  peerConnection_().getReceivers().forEach(function(receiver) {
    if (receiver == null)
      throw failTest('receiver is null or undefined.');
    if (receiver.track == null)
      throw failTest('receiver.track is null or undefined.');
    if (receiver.getContributingSources().length != 0)
      throw failTest('receiver.getContributingSources() is not empty.');
    receivers.add(receiver);
    receiverTracks.add(receiver.track);
  });
  if (receiverTracks.size != receivers.size)
    throw failTest('receiverTracks.size != receivers.size');

  returnToTest('ok-receivers-verified');
}

/**
 * Creates an audio and video track and adds them to the peer connection using
 * |addTrack|. They are added with or without a stream in accordance with
 * |streamArgumentType|.
 *
 * Returns
 * "ok-<audio stream id> <audio track id> <video stream id> <video track id>" on
 * success. If no stream is backing up the track, <stream id> is "null".
 *
 * @param {string} streamArgumentType Must be one of the following values:
 * 'no-stream' - The tracks are added without an associated stream.
 * 'shared-stream' - The tracks are added with the same associated stream.
 * 'individual-streams' - A stream is created for each track.
 */
function createAndAddAudioAndVideoTrack(streamArgumentType) {
  if (streamArgumentType !== 'no-stream' &&
      streamArgumentType !== 'shared-stream' &&
      streamArgumentType !== 'individual-streams')
    throw failTest('Unsupported streamArgumentType.');
  navigator.getUserMedia({ audio: true, video: true },
      function(stream) {
        let audioStream = undefined;
        if (streamArgumentType !== 'no-stream')
          audioStream = new MediaStream();

        let audioTrack = stream.getAudioTracks()[0];
        let audioSender =
            audioStream ? peerConnection_().addTrack(audioTrack, audioStream)
                        : peerConnection_().addTrack(audioTrack);
        if (!audioSender || audioSender.track != audioTrack)
          throw failTest('addTrack did not return a sender with the track.');

        let videoStream = undefined;
        if (streamArgumentType === 'shared-stream') {
          videoStream = audioStream;
        } else if (streamArgumentType === 'individual-streams') {
          videoStream = new MediaStream();
        }

        let videoTrack = stream.getVideoTracks()[0];
        let videoSender =
            videoStream ? peerConnection_().addTrack(videoTrack, videoStream)
                        : peerConnection_().addTrack(videoTrack);
        if (!videoSender || videoSender.track != videoTrack)
          throw failTest('addTrack did not return a sender with the track.');

        let audioStreamId = audioStream ? audioStream.id : 'null';
        let videoStreamId = videoStream ? videoStream.id : 'null';
        returnToTest('ok-' + audioStreamId + ' ' + audioTrack.id
                     + ' ' + videoStreamId + ' ' + videoTrack.id);
      },
      function(error) {
        throw failTest('getUserMedia failed: ' + error);
      });
}

/**
 * Calls |removeTrack| with the first sender that has the track with |trackId|
 * and verifies the SDP is updated accordingly.
 *
 * Returns "ok-sender-removed" on success.
 */
function removeTrack(trackId) {
  let sender = null;
  let otherSenderHasTrack = false;
  peerConnection_().getSenders().forEach(function(s) {
    if (s.track && s.track.id == trackId) {
      if (!sender)
        sender = s;
      else
        otherSenderHasTrack = true;
    }
  });
  if (!sender)
    throw failTest('There is no sender for track ' + trackId);
  peerConnection_().removeTrack(sender);
  if (sender.track)
    throw failTest('sender.track was not nulled by removeTrack.');
  returnToTest('ok-sender-removed');
}

/**
 * Returns "ok-stream-with-track-found" or "ok-stream-with-track-not-found".
 * If |streamId| is null then any stream having a track with |trackId| will do.
 */
function hasLocalStreamWithTrack(streamId, trackId) {
  if (hasStreamWithTrack(
          peerConnection_().getLocalStreams(), streamId, trackId)) {
    returnToTest('ok-stream-with-track-found');
    return;
  }
  returnToTest('ok-stream-with-track-not-found');
}

/**
 * Returns "ok-stream-with-track-found" or "ok-stream-with-track-not-found".
 * If |streamId| is null then any stream having a track with |trackId| will do.
 */
function hasRemoteStreamWithTrack(streamId, trackId) {
  if (hasStreamWithTrack(
          peerConnection_().getRemoteStreams(), streamId, trackId)) {
    returnToTest('ok-stream-with-track-found');
    return;
  }
  returnToTest('ok-stream-with-track-not-found');
}

/**
 * Returns "ok-sender-with-track-found" or "ok-sender-with-track-not-found".
 */
function hasSenderWithTrack(trackId) {
  if (hasSenderOrReceiverWithTrack(peerConnection_().getSenders(), trackId)) {
    returnToTest('ok-sender-with-track-found');
    return;
  }
  returnToTest('ok-sender-with-track-not-found');
}

/**
 * Returns "ok-receiver-with-track-found" or "ok-receiver-with-track-not-found".
 */
function hasReceiverWithTrack(trackId) {
  if (hasSenderOrReceiverWithTrack(peerConnection_().getReceivers(), trackId)) {
    returnToTest('ok-receiver-with-track-found');
    return;
  }
  returnToTest('ok-receiver-with-track-not-found');
}

// TODO(hbos): Make this a web platform test instead. https://crbug.com/773472
function createReceiverWithSetRemoteDescription() {
  var pc = new RTCPeerConnection();
  var receivers = null;
  pc.setRemoteDescription(createOffer([msid('stream', 'track1')]))
      .then(() => {
        receivers = pc.getReceivers();
        if (receivers.length != 1)
          throw failTest('getReceivers() should return 1 receiver: ' +
                         receivers.length)
        if (!receivers[0].track)
          throw failTest('getReceivers()[0].track should have a value')
        returnToTest('ok');
      });
  receivers = pc.getReceivers();
  if (receivers.length != 0)
    throw failTest('getReceivers() should return 0 receivers: ' +
                   receivers.length)
}

// TODO(hbos): Make this a web platform test instead. https://crbug.com/773472
function switchRemoteStreamAndBackAgain() {
  let pc1 = new RTCPeerConnection();
  let firstStream0 = null;
  let firstTrack0 = null;
  pc1.setRemoteDescription(createOffer([msid('stream0', 'track0')]))
    .then(() => {
      firstStream0 = pc1.getRemoteStreams()[0];
      firstTrack0 = firstStream0.getTracks()[0];
      if (firstStream0.id != 'stream0')
        throw failTest('Unexpected firstStream0.id: ' + firstStream0.id);
      if (firstTrack0.id != 'track0')
        throw failTest('Unexpected firstTrack0.id: ' + firstTrack0.id);
      return pc1.setRemoteDescription(
          createOffer([msid('stream1', 'track1')]));
    }).then(() => {
      return pc1.setRemoteDescription(
          createOffer([msid('stream0', 'track0')]));
    }).then(() => {
      let secondStream0 = pc1.getRemoteStreams()[0];
      let secondTrack0 = secondStream0.getTracks()[0];
      if (secondStream0.id != 'stream0')
        throw failTest('Unexpected secondStream0.id: ' + secondStream0.id);
      if (secondTrack0.id != 'track0')
        throw failTest('Unexpected secondTrack0.id: ' + secondTrack0.id);
      if (secondTrack0 == firstTrack0)
        throw failTest('Expected a new track object with the same id');
      if (secondStream0 == firstStream0)
        throw failTest('Expected a new stream object with the same id');
      returnToTest('ok');
    });
}

// TODO(hbos): Make this a web platform test instead. https://crbug.com/773472
function switchRemoteStreamWithoutWaitingForPromisesToResolve() {
  let pc = new RTCPeerConnection();
  let trackEventsFired = 0;
  let streamEventsFired = 0;
  pc.ontrack = (e) => {
    ++trackEventsFired;
    if (trackEventsFired == 1) {
      if (e.track.id != 'track0')
        throw failTest('Unexpected track id in first track event.');
      if (e.receiver.track != e.track)
        throw failTest('Unexpected receiver.track in first track event.');
      if (e.streams[0].id != 'stream0')
        throw failTest('Unexpected stream id in first track event.');
      // Because we did not wait for promises to resolve before calling
      // |setRemoteDescription| a second time, it may or may not have had an
      // effect here. This is inherently racey and we have to check if the
      // stream contains the track.
      if (e.streams[0].getTracks().length != 0) {
        if (e.streams[0].getTracks()[0] != e.track)
          throw failTest('Unexpected track in stream in first track event.');
      }
    } else if (trackEventsFired == 2) {
      if (e.track.id != 'track1')
        throw failTest('Unexpected track id in second track event.');
      if (e.receiver.track != e.track)
        throw failTest('Unexpected receiver.track in second track event.');
      if (e.streams[0].id != 'stream1')
        throw failTest('Unexpected stream id in second track event.');
      if (e.streams[0].getTracks()[0] != e.track)
        throw failTest('The track should belong to the stream in the second ' +
                       'track event.');
      if (streamEventsFired != trackEventsFired)
        throw failTest('All stream events should already have fired.');
      returnToTest('ok');
    }
  };
  pc.onaddstream = (e) => {
    ++streamEventsFired;
    if (streamEventsFired == 1) {
      if (e.stream.id != 'stream0')
        throw failTest('Unexpected stream id in first stream event.');
      // Because we did not wait for promises to resolve before calling
      // |setRemoteDescription| a second time, it may or may not have had an
      // effect here. This is inherently racey and we have to check if the
      // stream contains the track.
      if (e.stream.getTracks().length != 0) {
        if (e.stream.getTracks()[0].id != 'track0')
          throw failTest('Unexpected track id in first stream event.');
      }
    } else if (streamEventsFired == 2) {
      if (e.stream.id != 'stream1')
        throw failTest('Unexpected stream id in second stream event.');
      if (e.stream.getTracks()[0].id != 'track1')
        throw failTest('Unexpected track id in second stream event.');
    }
  };
  pc.setRemoteDescription(createOffer([msid('stream0', 'track0')]));
  pc.setRemoteDescription(createOffer([msid('stream1', 'track1')]));
}

// TODO(hbos): Make this a web platform test instead. https://crbug.com/773472
function trackSwitchingStream() {
  let pc = new RTCPeerConnection();
  let track = null;
  let stream1 = null;
  let stream2 = null;
  pc.setRemoteDescription(createOffer([msid('stream1', 'track1')]));
  pc.ontrack = (e) => {
    track = e.track;
    stream1 = e.streams[0];
    if (stream1.getTracks()[0] != track)
      throw failTest('stream1 does not contain track.');
    // This should update the associated set of streams for the existing
    // receiver for track.
    pc.setRemoteDescription(createOffer([msid('stream2', 'track')]));
    pc.ontrack = (e) => {
      let originalTrack = track;
      track = e.track;
      stream2 = e.streams[0];
      // TODO(hbos): A new track should not be created, track should simply
      // move. Fix this and update assertion. https://crbug.com/webrtc/8377
      if (track == originalTrack)
        throw failTest('A new track was not created.');
      if (stream1.getTracks().length != 0)
        throw failTest('stream1 is not empty.')
      if (!originalTrack.muted)
        throw failTest('Original track is not muted.');
      if (track.muted)
        throw failTest('New track is muted.');
      if (stream2.getTracks()[0] != track)
        throw failTest('stream2 does not contain track.');
      returnToTest('ok');
    };
  };
}

// TODO(hbos): Add a test that verifies a track that is added to two streams can
// be removed from just one of them. https://crbug.com/webrtc/8377

/**
 * Invokes the GC and returns "ok-gc".
 */
function collectGarbage() {
  gc();
  returnToTest('ok-gc');
}

// Internals.

function msid(stream, track) {
  return {
    stream: stream,
    track: track
  };
}

function createOffer(msids) {
  let msidLines = '';
  for (let i = 0; i < msids.length; ++i) {
    msidLines += 'a=msid:' + msids[i].stream + ' ' + msids[i].track + '\n';
  }
  return {
    type: 'offer',
    sdp: 'v=0\n' +
         'o=TestSDP 1337 0 IN IP4 0.0.0.0\n' +
         's=-\n' +
         't=0 0\n' +
         'a=fingerprint:sha-256 7A:69:A9:2B:ED:09:B8:88:D5:44:D6:9A:3F:B2:48:' +
             '6D:93:80:D1:39:AE:0C:3A:D5:89:EC:D8:39:95:62:9A:04\n' +
         'a=ice-options:trickle\n' +
         'a=msid-semantic:WMS *\n' +
         'm=audio 9 UDP/TLS/RTP/SAVPF 109 9 0 8 101\n' +
         'c=IN IP4 0.0.0.0\n' +
         'a=sendrecv\n' +
         'a=extmap:1/sendonly urn:ietf:params:rtp-hdrext:ssrc-audio-level\n' +
         'a=fmtp:109 maxplaybackrate=48000;stereo=1;useinbandfec=1\n' +
         'a=fmtp:101 0-15\n' +
         'a=ice-pwd:bcda8c8d9061b19b86f7798e262c60b5\n' +
         'a=ice-ufrag:4aeff5db\n' +
         'a=rtcp-mux\n' +
         'a=rtpmap:109 opus/48000/2\n' +
         'a=rtpmap:9 G722/8000/1\n' +
         'a=rtpmap:0 PCMU/8000\n' +
         'a=rtpmap:8 PCMA/8000\n' +
         'a=rtpmap:101 telephone-event/8000/1\n' +
         'a=setup:actpass\n' +
         'a=ssrc:1104456328 cname:{65ad23a5-22cd-2045-9154-0a745d951ccf}\n' +
         // These are all audio tracks because they're placed under the
         // "m=audio" section.
         msidLines
  };
}

/** @private */
function hasStreamWithTrack(streams, streamId, trackId) {
  for (let i = 0; i < streams.length; ++i) {
    let stream = streams[i];
    if (streamId && stream.id !== streamId)
      continue;
    let tracks = stream.getTracks();
    for (let j = 0; j < tracks.length; ++j) {
      let track = tracks[j];
      if (track.id == trackId) {
        return true;
      }
    }
  }
  return false;
}

/** @private */
function hasSenderOrReceiverWithTrack(sendersOrReceivers, trackId) {
  for (let i = 0; i < sendersOrReceivers.length; ++i) {
    if (sendersOrReceivers[i].track &&
        sendersOrReceivers[i].track.id === trackId) {
      return true;
    }
  }
  return false;
}

/** @private */
function arrayEquals_(a, b) {
  if (a == null)
    return b == null;
  if (a.length != b.length)
    return false;
  for (let i = 0; i < a.length; ++i) {
    if (a[i] !== b[i])
      return false;
  }
  return true;
}

/** @private */
function setEquals_(a, b) {
  if (a == null)
    return b == null;
  if (a.size != b.size)
    return false;
  a.forEach(function(value) {
    if (!b.has(value))
      return false;
  });
  return true;
}
