// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {
window.testRunner = {};
window.testRunner.isDone = false;

testRunner.waitUntilDone = function() {};
testRunner.dumpAsText = function() {};
testRunner.notifyDone = function() {
  this.isDone = true;
};

testRunner.supportTracing = true;

// If this is true, blink_perf tests is put on paused waiting for tracing to
// be started. |scheduleTestRun| should be invoked after tracing is started
// to continue blink perf test.
testRunner.isWaitingForTracingStart = false;

testRunner.startTracing = function(tracingCategories, scheduleTestRun) {
  this.tracingCategories = tracingCategories;
  this.scheduleTestRun = scheduleTestRun;
  this.isWaitingForTracingStart = true;
}

testRunner.stopTracingAndMeasure = function(traceEventsToMeasure, callback) {
  testRunner.traceEventsToMeasure = traceEventsToMeasure;
  callback();
}

window.GCController = {};

GCController.collect = function() {
  gc();
};
GCController.collectAll = function() {
  for (var i = 0; i < 7; ++i)
    gc();
};
})();
