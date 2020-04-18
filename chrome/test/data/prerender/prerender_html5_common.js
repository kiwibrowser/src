// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Common js for the HTML5_* tests. The following variables need to be defined
// before this js is included:
//   - 'willPlay' - indicates if the media is expected to start playing during
//      the test.
//   - 'testNetworkEvents' - if set to true, the test will test for the
//     loadstart and stalled events. NOTE that since the loadstart event fires
//     very early, to test for it reliably, the source of the media tag
//     should be added after this script is included or add
//     'onLoadStart=mediEventHandler' as an attribute to the media element.
//     Also to reliably test the stalled event, the the test should wait for the
//     prerendered page's title to change to "READY" before calling
//     DidPrerenderPass.

function assert(bool) {
  if (!bool)
    throw new Error('Assert Failed.');
}

var canPlaySeen = false;
var playingSeen = false;
var canPlayThroughSeen = false;
var loadStartSeen = false;
var stalledSeen = false;
var hasError = false;

assert(typeof(willPlay) != 'undefined');
assert(typeof(testNetworkEvents) != 'undefined');

var mediaEl = document.getElementById("mediaEl");

function mediaEventHandler(e) {
  switch (e.type) {
    case 'canplay':
      canPlaySeen = true;
      break;
    case 'playing':
      assert(canPlaySeen);
      playingSeen = true;
      break;
    case 'canplaythrough':
      assert(canPlaySeen);
      canPlayThroughSeen = true;
      break;
    case 'error':
      hasError = true;
      break;
    case 'loadstart':
      loadStartSeen = true;
      break;
    case 'stalled':
      assert(loadStartSeen);
      stalledSeen = true;
      if (testNetworkEvents) {
        document.title = 'READY';
      }
      break;
  }

  var progressDone = (willPlay && canPlayThroughSeen && playingSeen) ||
      (!willPlay && canPlayThroughSeen && !playingSeen);

  if (progressDone)
    document.title = 'PASS';
}

mediaEl.addEventListener('playing', mediaEventHandler, false);
mediaEl.addEventListener('canplay', mediaEventHandler, false);
mediaEl.addEventListener('canplaythrough', mediaEventHandler, false);
mediaEl.addEventListener('error', mediaEventHandler, false);

if (testNetworkEvents) {
  mediaEl.addEventListener('stalled', mediaEventHandler, false);
  mediaEl.addEventListener('loadstart', mediaEventHandler, false);
}

// TODO(shishir): Remove this once http://crbug.com/130788 is fixed.
function printDebugInfo() {
  console.log("\ncanPlaySeen: " + canPlaySeen);
  console.log("playingSeen: " + playingSeen);
  console.log("canPlayThroughSeen: " + canPlayThroughSeen);
  console.log("loadStartSeen: " + loadStartSeen);
  console.log("stalledSeen: " + stalledSeen);
  console.log("hasError: " + hasError + "\n");
}
setInterval(printDebugInfo, 5000);

function DidPrerenderPass() {
  // The media should not have started at this point.
  return !canPlaySeen && !playingSeen && !hasError &&
      mediaEl.currentTime == 0 &&
      mediaEl.readyState == mediaEl.HAVE_NOTHING &&
      (!testNetworkEvents || stalledSeen);
}

function DidDisplayPass() {
  // The actual test is done via the TitleWatcher.
  return true;
}
