// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var spinner;
var controller;

function waitForMutation(target) {
  return new Promise(function(fulfill, reject) {
    var observer = new MutationObserver(function(mutations) {
      observer.disconnect();
      fulfill();
    });
    observer.observe(target, {attributes: true});
  });
}

function setUp() {
  spinner = document.querySelector('#spinner');
  controller = new SpinnerController(spinner);
  // Set the duration to 100ms, which is short enough, but also long enough
  // to happen later than 0ms timers used in test cases.
  controller.setBlinkDurationForTesting(100);
}

function testBlink(callback) {
  assertTrue(spinner.hidden);
  controller.blink();

  return reportPromise(
    waitForMutation(spinner).then(function() {
      assertFalse(spinner.hidden);
      return waitForMutation(spinner);
    }).then(function() {
      assertTrue(spinner.hidden);
    }), callback);
}

function testShow(callback) {
  assertTrue(spinner.hidden);
  var hideCallback = controller.show();

  return reportPromise(
    waitForMutation(spinner).then(function() {
      assertFalse(spinner.hidden);
      return new Promise(function(fulfill, reject) {
        setTimeout(fulfill, 0);
      });
    }).then(function() {
      assertFalse(spinner.hidden);  // It should still be hidden.
      // Call asynchronously, so the mutation observer catches the change.
      setTimeout(hideCallback, 0);
      return waitForMutation(spinner);
    }).then(function() {
      assertTrue(spinner.hidden);
    }), callback);
}

function testShowDuringBlink(callback) {
  assertTrue(spinner.hidden);
  controller.blink();
  var hideCallback = controller.show();

  return reportPromise(
    waitForMutation(spinner).then(function() {
      assertFalse(spinner.hidden);
      return new Promise(function(fulfill, reject) {
        setTimeout(fulfill, 0);
      });
    }).then(function() {
      assertFalse(spinner.hidden);
      hideCallback();
      return new Promise(function(fulfill, reject) {
        setTimeout(fulfill, 0);
      });
    }).then(function() {
      assertFalse(spinner.hidden);
      return waitForMutation(spinner);
    }).then(function() {
      assertTrue(spinner.hidden);
    }), callback);
}

function testStackedShows(callback) {
  assertTrue(spinner.hidden);

  var hideCallbacks = [];
  hideCallbacks.push(controller.show());
  hideCallbacks.push(controller.show());

  return reportPromise(
    waitForMutation(spinner).then(function() {
      assertFalse(spinner.hidden);
      return new Promise(function(fulfill, reject) {
        setTimeout(fulfill, 0);
      });
    }).then(function() {
      assertFalse(spinner.hidden);
      hideCallbacks[1]();
      return new Promise(function(fulfill, reject) {
        setTimeout(fulfill, 0);
      });
    }).then(function() {
      assertFalse(spinner.hidden);
      // Call asynchronously, so the mutation observer catches the change.
      setTimeout(hideCallbacks[0], 0);
      return waitForMutation(spinner);
    }).then(function() {
      assertTrue(spinner.hidden);
    }), callback);
}
