// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// TODO(luoe): once BigInts are on by default, merge this test into
// http/tests/devtools/startup/console/console-format-startup.js

(async function() {
  await TestRunner.setupStartupTest('resources/console-format-startup-bigint.html');

  TestRunner.addResult('Tests console logging for messages with BigInts that happen before DevTools is open.\n');

  await TestRunner.loadModule('console_test_runner');
  await TestRunner.showPanel('console');

  TestRunner.hideInspectorView();
  ConsoleTestRunner.expandConsoleMessages(finish);
  function finish() {
    ConsoleTestRunner.dumpConsoleMessagesIgnoreErrorStackFrames();
    TestRunner.completeTest();
  }
})();
