// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var errorReported = false;

var testStartSessionErrorReport = function() {
  function onSessionError(errorSinkId, errorMessage){
    chrome.test.assertEq(1, errorSinkId);
    errorReported = true;
  };
  chrome.displaySource.onSessionErrorOccured.addListener(onSessionError);

  function onSessionTerminated(terminatedSink) {
    chrome.test.assertEq(1, terminatedSink);
    chrome.test.assertFalse(errorReported);
    chrome.test.succeed("SessionTerminated Received");
  };
  chrome.displaySource.onSessionTerminated.addListener(onSessionTerminated);

  chrome.tabs.getCurrent(function(tab) {
    var sink_id = 1;

    // If the test case does not provide height, width and frame properties,
    // the captured stream is 640x480 30 fps by default.
    // But WiFi Display requires 60 fps for this resolution.
    // Thus, min and max frame rates should be explicitly defined.

    var video_constraints = {
          mandatory: {
              minFrameRate: 60,
              maxFrameRate: 60
          }
       };

    var constraints = {
        audio: false,
        video: true,
        videoConstraints: video_constraints,
    };

    function onStream(stream) {
      var session_info = {
          sinkId: sink_id,
          videoTrack: stream.getVideoTracks()[0],
          audioTrack: stream.getAudioTracks()[0],
          authenticationInfo: { method: "PIN", data: "1234" }
      };

      function onStarted() {
        if (chrome.runtime.error) {
          console.log('The Session to sink ' + sink_id
            + 'could not start, error: '
            + chrome.runtime.lastError.message);
          chrome.test.fail();
        } else {
          chrome.test.assertNoLastError();
        }
      }
      chrome.displaySource.startSession(session_info, onStarted);
    }
    chrome.tabCapture.capture(constraints, onStream);
  });
};

chrome.test.runTests([testStartSessionErrorReport]);
