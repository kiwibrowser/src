// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(
      `Tests that syntax errors in eval are logged into console, contains correct link and doesn't cause browser crash.\n`);
  await TestRunner.loadModule('console_test_runner');
  await TestRunner.showPanel('console');
  await TestRunner.evaluateInPagePromise(`
      function foo()
      {
          eval("}");
      }
      function boo()
      {
          eval("\\n}\\n//# sourceURL=boo.js");
      }
    `);

  TestRunner.evaluateInPage('setTimeout(foo, 0);', ConsoleTestRunner.waitUntilMessageReceived.bind(this, step2));

  function step2() {
    TestRunner.evaluateInPage('setTimeout(boo, 0);', ConsoleTestRunner.waitUntilMessageReceived.bind(this, step3));
  }

  function step3() {
    ConsoleTestRunner.dumpConsoleMessages();
    TestRunner.completeTest();
  }
})();
