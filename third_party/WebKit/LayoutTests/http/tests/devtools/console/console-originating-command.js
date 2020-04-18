// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that console result has originating command associated with it.\n`);
  await TestRunner.loadModule('console_test_runner');
  await TestRunner.showPanel('console');

  ConsoleTestRunner.evaluateInConsole('1 + 1', step1);
  function step1() {
    ConsoleTestRunner.dumpConsoleMessages(true);
    TestRunner.completeTest();
  }
})();
