// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests the Timeline events for WebSocket\n`);
  await TestRunner.loadModule('performance_test_runner');
  await TestRunner.showPanel('timeline');
  await TestRunner.evaluateInPagePromise(`
      function performActions()
      {
          var ws = new WebSocket("ws://127.0.0.1:8880/simple");
          return new Promise((fulfill) => ws.onclose = fulfill);
      }
  `);

  await PerformanceTestRunner.invokeAsyncWithTimeline('performActions');

  PerformanceTestRunner.printTimelineRecordsWithDetails('WebSocketCreate');
  PerformanceTestRunner.printTimelineRecordsWithDetails('WebSocketSendHandshakeRequest');
  PerformanceTestRunner.printTimelineRecordsWithDetails('WebSocketReceiveHandshakeResponse');
  PerformanceTestRunner.printTimelineRecordsWithDetails('WebSocketDestroy');
  TestRunner.completeTest();
})();
