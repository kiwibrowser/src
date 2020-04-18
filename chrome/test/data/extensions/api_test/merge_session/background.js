// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var googleResponseReceived = false;
var googleRequestSent = false;
var nonGoogleResponseReceived = false;
var nonGoogleRequestSent = false;

function initGlobals() {
  googleResponseReceived = false;
  googleRequestSent = false;
  nonGoogleResponseReceived = false;
  nonGoogleRequestSent = false;
}

// Starts XHR requests - one for google.com and one kater for non-google.
function startXHRRequests(googlePageUrl, googlePageCheck,
                          nonGooglePageUrl, nonGooglePageCheck) {
  // Kick off google XHR first.
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    console.warn("xhr.onreadystatechange: " + xhr.readyState);
    if (xhr.readyState == 1) {
      startNonGoogleXHRRequests(nonGooglePageUrl, nonGooglePageCheck);
    } else if (xhr.readyState == 4) {  // done
      if (xhr.status == 200 &&
          xhr.responseText.indexOf('Hello Google') != -1) {
        googleResponseReceived = true;
        googlePageCheck();
      }
    }
  };
  xhr.open("GET", googlePageUrl, true);
  xhr.send();
  googleRequestSent = true;

}

function startNonGoogleXHRRequests(nonGooglePageUrl, nonGooglePageCheck) {
  // Kick off non-google XHR next.
  var xhr2 = new XMLHttpRequest();
  xhr2.onreadystatechange = function() {
    console.warn("xhr2.onreadystatechange: " + xhr2.readyState);
    if (xhr2.readyState == 4) {  // done
      if (xhr2.status == 200 &&
          xhr2.responseText.indexOf('SomethingElse') != -1) {
        // Google response should not have been received before non-google
        // XHR since it must be blocked by the throttle side right now.
        chrome.test.sendMessage('non-google-xhr-received');
        nonGoogleResponseReceived = true;
        nonGooglePageCheck();
      }
    }
  };
  xhr2.open("GET", nonGooglePageUrl, true);
  xhr2.send();
  nonGoogleRequestSent = true;
  return true;
}

function googlePageThrottleCheck() {
  // Google response should not have been received before non-google
  // XHR since it must be blocked by the throttle side right now.
  if (nonGoogleResponseReceived)
    chrome.test.succeed();
  else
    chrome.test.fail();
}

function nonGooglePageThrottleCheck() {
}
// Performs test that will verify if XHR request had completed prematurely.
function startThrottledTests(googlePageUrl,nonGooglePageUrl) {
  chrome.test.runTests([function testXHRThrottle() {
    initGlobals();
    startXHRRequests(googlePageUrl, googlePageThrottleCheck,
                     nonGooglePageUrl, nonGooglePageThrottleCheck);
  }]);
  return true;
}

function googlePageNoThrottleCheck() {
  console.warn("googlePageNoThrottleCheck: " + googleResponseReceived);
}

function nonGooglePageNoThrottleCheck() {
  // Non-google response should be received only after google request
  // since there is no throttle now.
  if (googleResponseReceived) {
    chrome.test.succeed();
  } else {
    console.warn("nonGooglePageNoThrottleCheck: " + googleResponseReceived);
    chrome.test.fail();
  }
}

// Performs test that will verify that XHR will complete in order they were
// issued since there is no throttle to change that since this test is running
// after merge session is completed.
function startNonThrottledTests(googlePageUrl, nonGooglePageUrl) {
  chrome.test.runTests([function testWithNoXHRThrottle() {
    initGlobals();
    startXHRRequests(googlePageUrl, googlePageNoThrottleCheck,
                     nonGooglePageUrl, nonGooglePageNoThrottleCheck);
  }]);
  return true;
}
