// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Waits until content sampled from |stream| indicates a success or fail status
// from an off-screen tab page that renders a green (success) or red (fail)
// color fill.
function waitForGreenOrRedTestResultAndEndTest(stream) {
  waitForAnExpectedColor(stream, [[255, 0, 0], [0, 255, 0]], 64,
      function (result) {
        stopAllTracks(stream);
        if (result == 1) {
          chrome.test.succeed();
        } else {
          // Fail after 3 seconds to allow examining the debug error message.
          setTimeout(function () { chrome.test.fail(); }, 3000);
        }
      }
  );
}

function waitForTabToCloseAndEndTest(stream) {
  let check = () => {
    if (!stream.getTracks().find(track => track.readyState != 'ended')) {
      chrome.test.succeed();
    }
  };
  check();
  stream.getTracks().forEach(track => {
    track.onended = check;
  });
}

chrome.test.runTests([
  function cannotAccessLocalResources() {
    chrome.tabCapture.captureOffscreenTab(
      makeDataUriFromDocument(makeOffscreenTabTestDocument(
          'function failIfIsLoadable(url, runNextTest) {\n' +
          '  var request = new XMLHttpRequest();\n' +
          '  request.open("GET", url);\n' +
          '  request.addEventListener("load", function () {\n' +
          '    setDebugMessage(url);\n' +
          '    setFillColor(redColor);\n' +
          '  });\n' +
          '  request.addEventListener("error", function () {\n' +
          '    setTimeout(runNextTest, 0);\n' +
          '  });' +
          '  request.send();\n' +
          '}\n' +
          '\n' +
          'failIfIsLoadable("file://something", function () {\n' +
          '  failIfIsLoadable("chrome-extension://' + chrome.runtime.id +
                             '/offscreen_evil_tests.js", function() {\n' +
          '    setFillColor(greenColor);\n' +
          '  });\n' +
          '});')),
      getCaptureOptions(),
      waitForGreenOrRedTestResultAndEndTest);
  },

  function cannotOpenNewTabsOrDialogs() {
    chrome.tabCapture.captureOffscreenTab(
      makeDataUriFromDocument(makeOffscreenTabTestDocument(
          'var wnd = window.open("data:text/html;charset=UTF-8,NOT SEEN");\n' +
          'if (wnd) {\n' +
          '  setDebugMessage("window.open() succeeded");\n' +
          '  setFillColor(redColor);\n' +
          '} else {\n' +
          '  var result = window.confirm("Can you see me?");\n' +
          '  if (result) {\n' +
          '    setDebugMessage("positive confirm result");\n' +
          '    setFillColor(redColor);\n' +
          '  } else {\n' +
          '    setFillColor(greenColor);\n' +
          '  }\n' +
          '}')),
      getCaptureOptions(),
      waitForGreenOrRedTestResultAndEndTest);
  },

  function cannotGetUserMedia() {
    chrome.tabCapture.captureOffscreenTab(
      makeDataUriFromDocument(makeOffscreenTabTestDocument(
          'navigator.webkitGetUserMedia(\n' +
          '  {video: true},\n' +
          '  function onSuccess(stream) { setFillColor(redColor); },\n' +
          '  function onFailure(error) { setFillColor(greenColor); });')),
      getCaptureOptions(),
      waitForGreenOrRedTestResultAndEndTest);
  },

  function cannotNavigateWhenPresenting() {
    const captureOptions = getCaptureOptions();
    captureOptions.presentationId = 'presentation_id';
    chrome.tabCapture.captureOffscreenTab(
      makeDataUriFromDocument(
        '<html><script>' +
        'window.location = "http://example.com/some_url.html";' +
        '</script></html>'),
      captureOptions,
      waitForTabToCloseAndEndTest);
    // NOTE: If this test times out, it means that one of the following did not
    // happen:
    // 1. page loaded
    // 2. tab capture began
    // 3. page attempted to navigate
    // 4. page was closed by offscreen_tab.cc
    // 5. MediaStreamTracks were ended
  },

  // NOTE: Before adding any more tests, the maximum off-screen tab limit would
  // have to be increased (or a design issue resolved).  This is because
  // off-screen tabs are not closed the instant the LocalMediaStream is stopped,
  // but approximately 1 second later.
]);
