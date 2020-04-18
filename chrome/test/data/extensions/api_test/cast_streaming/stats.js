// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var rtpStream = chrome.cast.streaming.rtpStream;
var tabCapture = chrome.tabCapture;
var udpTransport = chrome.cast.streaming.udpTransport;
var createSession = chrome.cast.streaming.session.create;
var pass = chrome.test.callbackPass;

chrome.test.runTests([
  function getStats() {
    console.log("[TEST] getStats");
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
        var videoParams = rtpStream.getSupportedParams(videoId)[0];
        chrome.test.assertTrue(!!audioParams.payload.codecName);
        chrome.test.assertTrue(!!videoParams.payload.codecName);
        udpTransport.setDestination(udpId,
                                    {address: "127.0.0.1", port: 2344});
        rtpStream.onStarted.addListener(
            stateMachine.onStarted.bind(stateMachine));
        stateMachine.onAllStarted =
            pass(function(audioId, videoId) {
          console.log("Enabling logging.");
          rtpStream.toggleLogging(audioId, true);
          rtpStream.toggleLogging(videoId, true);
          console.log("Stopping.");
          rtpStream.stop(audioId);
          rtpStream.stop(videoId);
        }.bind(null, audioId, videoId));
        rtpStream.onStopped.addListener(
            stateMachine.onStopped.bind(stateMachine));
        stateMachine.onAllStopped =
            pass(function(audioId, videoId) {
          rtpStream.getStats(audioId,
              stateMachine.onGotStats.bind(stateMachine, audioId));
          rtpStream.getStats(videoId,
              stateMachine.onGotStats.bind(stateMachine, videoId));
        }.bind(null, audioId, videoId));
        stateMachine.onGotAllLogs =
            pass(function(stream, audioId, videoId, udpId) {
          console.log("Disabling logging.");
          rtpStream.toggleLogging(audioId, false);
          rtpStream.toggleLogging(videoId, false);
          console.log("Destroying.");
          rtpStream.destroy(audioId);
          rtpStream.destroy(videoId);
          udpTransport.destroy(udpId);
          chrome.test.assertTrue(!!audioParams.payload.codecName);
          chrome.test.assertTrue(!!videoParams.payload.codecName);
          chrome.test.succeed();
        }.bind(null, stream, audioId, videoId, udpId));
        rtpStream.start(audioId, audioParams);
        rtpStream.start(videoId, videoParams);
      }.bind(null, stream)));
    }));
  },
]);
