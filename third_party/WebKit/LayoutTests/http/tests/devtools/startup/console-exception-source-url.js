// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  await TestRunner.setupStartupTest('resources/console-exception-source-url.html');
  TestRunner.addResult(
      `Tests that when exception happens before inspector is open source url is correctly shown in console.\n`);
  await TestRunner.loadModule('console_test_runner');

  ConsoleTestRunner.expandConsoleMessages();
  ConsoleTestRunner.dumpConsoleMessages();
  TestRunner.addResult('TEST COMPLETE');
  TestRunner.completeTest();
})();
