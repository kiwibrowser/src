// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult("Tests that console is cleared upon requestClearMessages call.\n");

  await TestRunner.showPanel("console");
  await TestRunner.loadModule("console_test_runner");

  await TestRunner.evaluateInPagePromise(`
    console.log("one");
    console.log("two");
    console.log("three");
  `);
  TestRunner.addResult("=== Before clear ===");
  ConsoleTestRunner.dumpConsoleMessages();

  Console.ConsoleView.clearConsole();
  TestRunner.deprecatedRunAfterPendingDispatches(callback);
  function callback() {
    TestRunner.addResult("=== After clear ===");
    ConsoleTestRunner.dumpConsoleMessages();
    TestRunner.completeTest();
  }
})();