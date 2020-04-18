// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(
      `Test that debugger will pause in all event listeners when corresponding breakpoint is set. Bug 77331.\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');
  await TestRunner.loadHTML(`
      <input type="button" id="test">
    `);
  await TestRunner.evaluateInPagePromise(`
      function listener1()
      {
      }

      function listener2()
      {
      }

      function listener3()
      {
      }

      function addListenerAndClick()
      {
          var element = document.getElementById("test");
          element.addEventListener("click", listener1, true);
          element.addEventListener("click", listener2, true);
          document.body.addEventListener("click", listener3, true);
          document.body.addEventListener("click", listener3, false);
          element.click();
      }
  `);

  SourcesTestRunner.runDebuggerTestSuite([function testClickBreakpoint(next) {
    SourcesTestRunner.setEventListenerBreakpoint('listener:click', true);
    SourcesTestRunner.waitUntilPaused(paused1);
    TestRunner.evaluateInPageWithTimeout('addListenerAndClick()');

    function paused1(callFrames) {
      SourcesTestRunner.captureStackTrace(callFrames);
      SourcesTestRunner.resumeExecution(resumed1);
    }

    function resumed1() {
      SourcesTestRunner.waitUntilPaused(paused2);
    }

    function paused2(callFrames) {
      SourcesTestRunner.captureStackTrace(callFrames);
      SourcesTestRunner.resumeExecution(resumed2);
    }

    function resumed2() {
      SourcesTestRunner.waitUntilPaused(paused3);
    }

    function paused3(callFrames) {
      SourcesTestRunner.captureStackTrace(callFrames);
      SourcesTestRunner.resumeExecution(resumed3);
    }

    function resumed3() {
      SourcesTestRunner.waitUntilPaused(paused4);
    }

    function paused4(callFrames) {
      SourcesTestRunner.captureStackTrace(callFrames);
      SourcesTestRunner.setEventListenerBreakpoint('listener:click', false);
      SourcesTestRunner.resumeExecution(next);
    }
  }]);
})();
