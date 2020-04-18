// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Fake APIs
var chrome;

/** @type {analytics.Tracker.Hit} The last hit received. */
var lastHit = null;

/** @type {!analytics.Tracker} */
var tracker;

function setUp() {
  setupFakeChromeAPIs();

  // Set a fake tracking ID so the tests aren't actually sending analytics.
  metrics.TRACKING_IDS[chrome.runtime.id] = 'UA-XXXXX-XX';
  tracker  = metrics.getTracker();

  // Make a filter that logs the last received hit.
  tracker.addFilter(
      /** @param {!analytics.Tracker.Hit} hit */
      function(hit) {
        // Log the hit.
        lastHit = hit;
      });
  // Reset the last logged hit.
  lastHit = null;
}

// Verifies that analytics logging occurs when fileManagerPrivate.isUMAEnabled
// returns true.
function testBasicLogging(callback) {
  // Simulate UMA enabled, then check that hits are sent.
  chrome.fileManagerPrivate.umaEnabled = true;
  reportPromise(
      tracker.sendAppView('Test').addCallback(
          function() {
            assertTrue(!!lastHit);
          }),
      callback);
}

// Verifies that analytics logging does not occur when
// fileManagerPrivate.isUMAEnabled returns false.
function testUMADisabled(callback) {
  // Simulate UMA disabled, and verify that hits aren't sent.
  chrome.fileManagerPrivate.umaEnabled = false;
  reportPromise(
      tracker.sendAppView('Test').addCallback(
          function() {
            assertTrue(lastHit === null);
          }),
      callback);
}

// Verifies that toggling the UMA setting causes the user's analytics ID to be
// reset.
function testResetAnalyticsOnUMAToggle(callback) {
  var id0 = null;
  var id1 = null;

  // Simulate UMA enabled, and send a hit.
  chrome.fileManagerPrivate.umaEnabled = true;
  reportPromise(
      tracker.sendAppView('Test0')
          .then(
              function() {
                // Log the analytics ID that got sent.
                id0 = lastHit.getParameters().toObject()['clientId'];
                // Simulate UMA disabled, send another hit.
                chrome.fileManagerPrivate.umaEnabled = false;
                return tracker.sendAppView('Test1');
              })
          .then(
              function() {
                // Re-enable analytics, send another hit.
                chrome.fileManagerPrivate.umaEnabled = true;
                return tracker.sendAppView('Test2');
              })
          .then(
              function() {
                // Check that a subsequent hit has a different analytics ID.
                id1 = lastHit.getParameters().toObject()['clientId'];
                assertTrue(id0 !== id1);
              }),
      callback);
}

function setupFakeChromeAPIs() {
  chrome = {
    runtime: {
      getManifest: function() {
        return {
          version: 0.0
        };
      }
    },
    storage: {
      local: {
        // Analytics uses storage to store the enabled/disabled flag.  Hard-wire
        // the get method to always return true so analytics is jammed on for
        // the purposes of testing.
        get: function(data, cb) { cb(true); },
        set: function(data, cb) {}
      },
      onChanged: {
        addListener: function(cb) {}
      }
    },
    fileManagerPrivate: {
      umaEnabled: false,
      isUMAEnabled: function(cb) { cb(chrome.fileManagerPrivate.umaEnabled); }
    }
  };
}
