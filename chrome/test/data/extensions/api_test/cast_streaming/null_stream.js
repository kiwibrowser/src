// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var rtpStream = chrome.cast.streaming.rtpStream;
var tabCapture = chrome.tabCapture;
var udpTransport = chrome.cast.streaming.udpTransport;
var createSession = chrome.cast.streaming.session.create;
var pass = chrome.test.callbackPass;

chrome.test.runTests([
  function noVideo() {
    console.log("[TEST] noVideo");
    chrome.tabs.create({"url": "about:blank"}, pass(function(tab) {
      tabCapture.capture({audio: true, video: false},
                         pass(function(stream) {
        chrome.test.assertTrue(!!stream);
        createSession(stream.getAudioTracks()[0],
                      null,
                      pass(function(stream, audioId, videoId, udpId) {
          chrome.test.assertTrue(audioId > 0);
          chrome.test.assertTrue(videoId == null);
          chrome.test.assertTrue(udpId > 0);
          rtpStream.destroy(audioId);
          udpTransport.destroy(udpId);
        }.bind(null, stream)));
      }));
    }));
  },
  function noAudio() {
    console.log("[TEST] noAudio");
    chrome.tabs.create({"url": "about:blank"}, pass(function(tab) {
      tabCapture.capture({audio: false, video: true},
                         pass(function(stream) {
        chrome.test.assertTrue(!!stream);
        createSession(null,
                      stream.getVideoTracks()[0],
                      pass(function(stream, audioId, videoId, udpId) {
          chrome.test.assertTrue(audioId == null);
          chrome.test.assertTrue(videoId > 0);
          chrome.test.assertTrue(udpId > 0);
          rtpStream.destroy(videoId);
          udpTransport.destroy(udpId);
        }.bind(null, stream)));
      }));
    }));
  },
  function remotingSession() {
    console.log("[TEST] remotingSession");
    chrome.tabs.create({"url": "about:blank"}, pass(function(tab) {
      createSession(null, null, function(a, b, c) {});
    }));
  },
]);
