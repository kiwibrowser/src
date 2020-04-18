// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(
      `Tests the Timeline API instrumentation of multiple style recalc invalidations and ensures they are all collected on the paint event.\n`);
  await TestRunner.loadModule('performance_test_runner');
  await TestRunner.showPanel('timeline');
  await TestRunner.loadHTML(`
      <!DOCTYPE HTML>
      <style>
          .testHolder > .red { background-color: red; }
          .testHolder > .green { background-color: green; }
          .testHolder > .blue { background-color: blue; }
          .testHolder > .snow { background-color: snow; }
          .testHolder > .red .dummy { }
          .testHolder > .green .dummy { }
          .testHolder > .blue .dummy { }
          .testHolder > .snow .dummy { }
      </style>
      <div class="testHolder">
      <div id="testElementOne">PASS</div><div id="testElementTwo">PASS</div><div id="testElementThree">PASS</div>
      </div>
    `);
  await TestRunner.evaluateInPagePromise(`
      function multipleStyleRecalcsAndDisplay()
      {
          document.getElementById("testElementOne").className = "red";
          var forceStyleRecalc1 = document.body.offsetTop;
          document.getElementById("testElementOne").className = "snow";
          var forceStyleRecalc2 = document.body.offsetTop;
          return waitForFrame();
      }
  `);

  Runtime.experiments.enableForTest('timelineInvalidationTracking');
  await PerformanceTestRunner.invokeAsyncWithTimeline('multipleStyleRecalcsAndDisplay');

  PerformanceTestRunner.dumpInvalidations(
      TimelineModel.TimelineModel.RecordType.UpdateLayoutTree, 0, 'first style recalc');
  PerformanceTestRunner.dumpInvalidations(
      TimelineModel.TimelineModel.RecordType.UpdateLayoutTree, 1, 'second style recalc');
  PerformanceTestRunner.dumpInvalidations(TimelineModel.TimelineModel.RecordType.Paint, 0, 'first paint');
  var thirdRecalc =
      PerformanceTestRunner.findTimelineEvent(TimelineModel.TimelineModel.RecordType.UpdateLayoutTree, 2);
  TestRunner.assertTrue(thirdRecalc === undefined, 'There should be no additional style recalc records.');
  var secondPaint = PerformanceTestRunner.findTimelineEvent(TimelineModel.TimelineModel.RecordType.Paint, 1);
  TestRunner.assertTrue(secondPaint === undefined, 'There should be no additional paint records.');

  TestRunner.completeTest();
})();
