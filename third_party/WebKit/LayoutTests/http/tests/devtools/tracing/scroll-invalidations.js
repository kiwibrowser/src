// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests invalidations produced by scrolling a page with position: fixed elements.\n`);
  await TestRunner.loadModule('performance_test_runner');
  await TestRunner.showPanel('timeline');
  await TestRunner.loadHTML(`
    <div style="width: 400px; height: 2000px; background-color: grey"></div>
    <div style="position: fixed; left: 50px; top: 100px; width: 50px; height: 50px; background-color: rgba(255, 100, 100, 0.6)"></div>
  `);
  await TestRunner.evaluateInPagePromise(`
    function scrollAndDisplay() {
      scrollTo(0, 200);
      return new Promise(fulfill => testRunner.layoutAndPaintAsyncThen(fulfill));
    }
  `);

  Runtime.experiments.enableForTest('timelineInvalidationTracking');
  await PerformanceTestRunner.invokeAsyncWithTimeline('scrollAndDisplay');

  const event = PerformanceTestRunner.findTimelineEvent(TimelineModel.TimelineModel.RecordType.Paint);
  TestRunner.addArray(
      TimelineModel.InvalidationTracker.invalidationEventsFor(event), PerformanceTestRunner.InvalidationFormatters,
      '', 'Scroll invalidations');
  TestRunner.completeTest();
})();
