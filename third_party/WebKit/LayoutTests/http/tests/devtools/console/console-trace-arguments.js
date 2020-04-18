// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that console.trace dumps arguments alongside the stack trace.\n`);

  await TestRunner.loadModule('console_test_runner');
  await TestRunner.showPanel('console');
  await TestRunner.evaluateInPagePromise(`
    function a()
    {
      console.trace(1, 2, 3);
    }
  `);

  function callback() {
    ConsoleTestRunner.dumpConsoleMessages();
    TestRunner.completeTest();
  }
  TestRunner.evaluateInPage('setTimeout(a, 0)');
  ConsoleTestRunner.addConsoleSniffer(callback);
})();
