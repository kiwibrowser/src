// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests stopping in debugger in the worker.\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.loadModule('console_test_runner');
  await TestRunner.showPanel('sources');
  await TestRunner.evaluateInPagePromise(`
      function installWorker()
      {
          new Worker("resources/worker-source.js");
      }
  `);

  SourcesTestRunner.startDebuggerTest(step1);

  function step1() {
    TestRunner.evaluateInPage('installWorker()');
    SourcesTestRunner.waitUntilPaused(paused);
  }

  function paused(callFrames) {
    SourcesTestRunner.captureStackTrace(callFrames);
    SourcesTestRunner.resumeExecution(SourcesTestRunner.completeDebuggerTest);
  }
})();
