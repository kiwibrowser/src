// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests console.timeline and timelineEnd commands.\n`);
  await TestRunner.loadModule('console_test_runner');
  await TestRunner.loadModule('performance_test_runner');
  await TestRunner.showPanel('timeline');
  await TestRunner.evaluateInPagePromise(`
      function startStopTimeline()
      {
          console.timeStamp("timestamp 0");
          console.timeline("one");
          console.timeStamp("timestamp 1");
          console.timelineEnd("one");
          console.timeStamp("timestamp 2");
      }

      function startStopMultiple()
      {
          console.timeStamp("timestamp 0");
          console.timeline("one");
          console.timeStamp("timestamp 1");
          console.timeline("one");
          console.timeline("two");
          console.timeline("two");
          console.timelineEnd("two");
          console.timeStamp("timestamp 2");
          console.timelineEnd("one");
          console.timeStamp("timestamp 3");
          console.timelineEnd("two");
          console.timeStamp("timestamp 4");
          console.timelineEnd("one");
          console.timeStamp("timestamp 5");
      }

      function startMultiple()
      {
          console.timeStamp("timestamp 0");
          console.timeline("one");
          console.timeStamp("timestamp 1");
          console.timeline("two");
          console.timeStamp("timestamp 2");
      }

      function stopTwo()
      {
          console.timeStamp("timestamp 3");
          console.timelineEnd("two");
          console.timeStamp("timestamp 4");
      }

      function stopOne()
      {
          console.timeStamp("timestamp 5");
          console.timelineEnd("one");
          console.timeStamp("timestamp 6 - FAIL");
      }

      function stopUnknown()
      {
          console.timeStamp("timestamp 0");
          console.timeline("one");
          console.timeStamp("timestamp 1");
          console.timelineEnd("two");
          console.timeStamp("timestamp 2");
          console.timelineEnd("one");
          console.timeStamp("timestamp 3");
      }

      function startTimeline()
      {
          console.timeStamp("timestamp 0");
          console.timeline("one");
          console.timeStamp("timestamp 1");
          console.timeline("two");
          console.timeStamp("timestamp 2");
      }
  `);

  TestRunner.runTestSuite([
    async function testStartStopTimeline(next) {
      await PerformanceTestRunner.evaluateWithTimeline('startStopTimeline()');
      printTimelineAndTimestampEvents();
      next();
    },

    async function testStartStopMultiple(next) {
      await PerformanceTestRunner.evaluateWithTimeline('startStopMultiple()');
      printTimelineAndTimestampEvents();
      next();
    },

    async function testStartMultipleStopInsideEvals(next) {
      await PerformanceTestRunner.startTimeline();
      TestRunner.evaluateInPage('startMultiple()', step2);

      function step2() {
        TestRunner.evaluateInPage('stopTwo()', step3);
      }

      function step3() {
        TestRunner.evaluateInPage('stopOne()', step4);
      }

      async function step4() {
        await PerformanceTestRunner.stopTimeline();
        printTimelineAndTimestampEvents();
        next();
      }
    },

    async function testStopUnknown(next) {
      await PerformanceTestRunner.evaluateWithTimeline('stopUnknown()');
      printTimelineAndTimestampEvents();
      next();
    },

    async function testStartFromPanel(next) {
      await PerformanceTestRunner.evaluateWithTimeline('startStopTimeline()');
      printTimelineAndTimestampEvents();
      next();
    },

    async function testStopFromPanel(next) {
      await PerformanceTestRunner.evaluateWithTimeline('startTimeline()');
      printTimelineAndTimestampEvents();
      next();
    }
  ]);

  function printTimelineAndTimestampEvents() {
    PerformanceTestRunner.tracingModel().sortedProcesses().forEach(function(process) {
      process.sortedThreads().forEach(function(thread) {
        thread.events().forEach(function(event) {
          if (event.hasCategory(TimelineModel.TimelineModel.Category.Console))
            TestRunner.addResult(event.name);
          else if (event.name === TimelineModel.TimelineModel.RecordType.TimeStamp)
            TestRunner.addResult(event.args['data']['message']);
        });
      });
    });
  }
})();
