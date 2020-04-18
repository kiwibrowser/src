// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var rtpStream = chrome.cast.streaming.rtpStream;
var tabCapture = chrome.tabCapture;
var udpTransport = chrome.cast.streaming.udpTransport;
var createSession = chrome.cast.streaming.session.create;
var pass = chrome.test.callbackPass;

chrome.test.runTests([
  function stopNoStart() {
    console.log("[TEST] stopNoStart");
    tabCapture.capture({audio: true, video: true},
                       pass(function(stream) {
      console.log("Got MediaStream.");
      chrome.test.assertTrue(!!stream);
      createSession(stream.getAudioTracks()[0],
                    stream.getVideoTracks()[0],
                    pass(function(stream, audioId, videoId, udpId) {
        chrome.test.assertTrue(audioId > 0);
        chrome.test.assertTrue(videoId > 0);
        chrome.test.assertTrue(udpId > 0);
        rtpStream.stop(audioId);
        rtpStream.stop(videoId);
        rtpStream.destroy(audioId);
        rtpStream.destroy(videoId);
        udpTransport.destroy(udpId);
        chrome.test.succeed();
      }.bind(null, stream)));
    }));
  },
]);
