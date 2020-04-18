// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var rtpStream = chrome.cast.streaming.rtpStream;
var tabCapture = chrome.tabCapture;
var udpTransport = chrome.cast.streaming.udpTransport;
var createSession = chrome.cast.streaming.session.create;
var pass = chrome.test.callbackPass;

chrome.test.runTests([
  function rtpStreamStart() {
    console.log("[TEST] rtpStreamStart");
    tabCapture.capture({audio: true, video: true},
                       pass(function(stream) {
      console.log("Got MediaStream.");
      chrome.test.assertTrue(!!stream);
      createSession(stream.getAudioTracks()[0],
                    stream.getVideoTracks()[0],
                    pass(function(stream, audioId, videoId, udpId) {
        console.log("Starting.");
        var stateMachine = new TestStateMachine(stream,
                                                audioId,
                                                videoId,
                                                udpId);
        var audioParams = rtpStream.getSupportedParams(audioId)[0];
        chrome.test.assertTrue(!!audioParams.payload.codecName);
        var expectError = function(id, message) {
          chrome.test.assertEq("Destination not set.", message);
          chrome.test.succeed();
        }
        console.log("Starting RTP stream before setting destination.");
        rtpStream.onError.addListener(expectError);
        rtpStream.start(audioId, audioParams);
      }.bind(null, stream)));
    }));
  },
]);
