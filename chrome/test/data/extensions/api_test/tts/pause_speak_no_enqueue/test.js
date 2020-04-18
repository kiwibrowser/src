// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TTS api test for Chrome.
// browser_tests.exe --gtest_filter="TtsApiTest.*"

chrome.test.runTests([
  function testPauseCancel() {
    var callbacks = 0;
    chrome.tts.pause();
    chrome.tts.speak(
        'text 1',
        {
         'enqueue': true,
         'onEvent': function(event) {
            chrome.test.assertEq('cancelled', event.type);
            callbacks++;
         }
        },
        function() {
          chrome.test.assertNoLastError();
          callbacks++;
        });
    chrome.tts.speak(
        'text 2',
        {
         'enqueue': false,
         'onEvent': function(event) {
           chrome.test.assertEq('cancelled', event.type);
           callbacks++;
           if (callbacks == 4) {
             chrome.test.succeed();
           }
         }
        },
        function() {
          chrome.test.assertNoLastError();
          callbacks++;
        });
  }
]);
