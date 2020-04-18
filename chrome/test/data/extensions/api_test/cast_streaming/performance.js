// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Run a cast v2 mirroring session for 15 seconds.

chrome.test.runTests([
  function sendTestPatterns() {
    const kMaxFrameRate = 30;
    const kCallbackTimeoutMillis = 10000;
    const kTestRunTimeMillis = 15000;

    const rtpStream = chrome.cast.streaming.rtpStream;

    // The receive port changes between browser_test invocations, and is passed
    // as an query parameter in the URL.
    let recvPort;
    let autoThrottling;
    let aesKey;
    let aesIvMask;
    try {
      const params = window.location.search;
      recvPort = parseInt(params.match(/(\?|&)port=(\d+)/)[2]);
      chrome.test.assertTrue(recvPort > 0);
      autoThrottling = (params.match(/(\?|&)autoThrottling=(true|false)/)[2] ==
                            'true');
      aesKey = params.match(/(\?|&)aesKey=([0-9A-F]{32})/)[2];
      aesIvMask = params.match(/(\?|&)aesIvMask=([0-9A-F]{32})/)[2];
    } catch (err) {
      chrome.test.fail("Error parsing params -- " + err.message);
      return;
    }

    // Start capture and wait up to kCallbackTimeoutMillis for it to start.
    const startCapturePromise = new Promise((resolve, reject) => {
      const timeoutId = setTimeout(() => {
        reject(Error('chrome.tabCapture.capture() did not call back'));
      }, kCallbackTimeoutMillis);

      const captureOptions = {
        video: true,
        audio: true,
        videoConstraints: {
          mandatory: {
            minWidth: autoThrottling ? 320 : 1920,
            minHeight: autoThrottling ? 180 : 1080,
            maxWidth: 1920,
            maxHeight: 1080,
            maxFrameRate: kMaxFrameRate,
            enableAutoThrottling: autoThrottling,
          }
        }
      };
      chrome.tabCapture.capture(captureOptions, captureStream => {
        clearTimeout(timeoutId);
        if (captureStream) {
          console.log('Started tab capture.');
          resolve(captureStream);
        } else {
          if (chrome.runtime.lastError) {
            reject(chrome.runtime.lastError);
          } else {
            reject(Error('null stream'));
          }
        }
      });
    });

    // Then, start Cast Streaming and wait up to kCallbackTimeoutMillis for it
    // to start.
    const startStreamingPromise = startCapturePromise.then(captureStream => {
      return new Promise((resolve, reject) => {
        const timeoutId = setTimeout(() => {
          reject(Error(
            'chrome.cast.streaming.session.create() did not call back'));
        }, kCallbackTimeoutMillis);

        chrome.cast.streaming.session.create(
            captureStream.getAudioTracks()[0],
            captureStream.getVideoTracks()[0],
            (audioId, videoId, udpId) => {
          clearTimeout(timeoutId);

          try {
            chrome.cast.streaming.udpTransport.setDestination(
                udpId, { address: "127.0.0.1", port: recvPort } );
            rtpStream.onError.addListener(() => {
              chrome.test.fail('RTP stream error');
            });
            const audioParams = rtpStream.getSupportedParams(audioId)[0];
            audioParams.payload.aesKey = aesKey;
            audioParams.payload.aesIvMask = aesIvMask;
            rtpStream.start(audioId, audioParams);
            const videoParams = rtpStream.getSupportedParams(videoId)[0];
            videoParams.payload.clockRate = kMaxFrameRate;
            videoParams.payload.aesKey = aesKey;
            videoParams.payload.aesIvMask = aesIvMask;
            rtpStream.start(videoId, videoParams);

            console.log('Started Cast Streaming.');
            resolve([captureStream, audioId, videoId, udpId]);
          } catch (error) {
            reject(error);
          }
        });
      });
    });

    // Then, let the test run for kTestRunTimeMillis, and then shut down the RTP
    // streams and MediaStreamTracks.
    const doneTestingPromise = startStreamingPromise.then(
        ([captureStream, audioId, videoId, udpId]) => {
      return new Promise(resolve => {
        console.log(`Running test for ${kTestRunTimeMillis} ms.`);
        setTimeout(resolve, kTestRunTimeMillis);
      }).then(() => {
        rtpStream.stop(audioId);
        rtpStream.stop(videoId);
        rtpStream.destroy(audioId);
        rtpStream.destroy(videoId);

        chrome.cast.streaming.udpTransport.destroy(udpId);

        const tracks = captureStream.getTracks();
        for (let i = 0; i < tracks.length; ++i) {
          tracks[i].stop();
        }
      });
    });

    // If all of the above completed without error, the test run has succeeded.
    // Otherwise, flag that the test has failed with the cause.
    doneTestingPromise.then(() => {
      chrome.test.succeed();
    }).catch(error => {
      if (typeof error === 'object' &&
          ('stack' in error || 'message' in error)) {
        chrome.test.fail(error.stack || error.message);
      } else {
        chrome.test.fail(String(error));
      }
    });
  }
]);
