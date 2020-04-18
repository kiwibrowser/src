// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests the Timeline events for Timers\n`);
  await TestRunner.loadModule('performance_test_runner');
  await TestRunner.showPanel('timeline');
  await TestRunner.evaluateInPagePromise(`
      function performActions()
      {
          var callback;
          var promise = new Promise((fulfill) => callback = fulfill);
          var timerOne = setTimeout("1 + 1", 10);
          var timerTwo = setInterval(intervalTimerWork, 20);
          var iteration = 0;

          function intervalTimerWork()
          {
              if (++iteration < 2)
                  return;
              clearInterval(timerTwo);
              callback();
          }
          return promise;
      }
  `);

  UI.panels.timeline._disableCaptureJSProfileSetting.set(true);
  await PerformanceTestRunner.invokeAsyncWithTimeline('performActions');

  PerformanceTestRunner.printTimelineRecordsWithDetails('TimerInstall');
  PerformanceTestRunner.printTimelineRecordsWithDetails('TimerFire');
  PerformanceTestRunner.printTimelineRecordsWithDetails('TimerRemove');
  PerformanceTestRunner.printTimelineRecords('FunctionCall');
  PerformanceTestRunner.printTimelineRecordsWithDetails('EvaluateScript');
  TestRunner.completeTest();
})();
