// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  await TestRunner.setupStartupTest('resources/console-promise-reject-and-handle.html');
  TestRunner.addResult(`Tests that evt.preventDefault() in window.onunhandledrejection suppresses console output.\n`);
  await TestRunner.loadModule('console_test_runner');

  ConsoleTestRunner.expandConsoleMessages();
  TestRunner.addResult('----console messages start----');
  ConsoleTestRunner.dumpConsoleMessages();
  TestRunner.addResult('----console messages end----');
  TestRunner.completeTest();
})();
