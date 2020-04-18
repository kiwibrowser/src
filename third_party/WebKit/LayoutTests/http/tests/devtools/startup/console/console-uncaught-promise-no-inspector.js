// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  await TestRunner.setupStartupTest('resources/console-uncaught-promise-no-inspector.html');

  TestRunner.addResult(
      `Tests that uncaught promise rejection messages have line numbers when the inspector is closed and stack traces are not collected.\n`);
  await TestRunner.loadModule('console_test_runner');

  ConsoleTestRunner.expandConsoleMessages(onExpanded);

  function onExpanded() {
    ConsoleTestRunner.dumpConsoleMessagesIgnoreErrorStackFrames();
    TestRunner.completeTest();
  }
})();
