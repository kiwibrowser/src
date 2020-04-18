// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that cross origin errors are logged with source url and line number.\n`);
  await TestRunner.loadModule('console_test_runner');
  await TestRunner.loadHTML(`
      <iframe src="http://localhost:8000/devtools/resources/cross-origin-iframe.html"></iframe>
    `);
  await TestRunner.evaluateInPagePromise(`
      function accessFrame()
      {
          // Should fail.
          try {
              var host = frames[0].location.host;
          } catch (e) {}

          // Should fail.
          try {
              frames[0].location.reload();
          } catch (e) {}

          // Should fail.
          frames[0].postMessage("fail", "http://127.0.0.1:8000");
      }
  `);

  ConsoleTestRunner.addConsoleSniffer(finish);
  Common.settingForTest('monitoringXHREnabled').set(true);
  TestRunner.evaluateInPage('accessFrame()');

  function finish() {
    Common.settingForTest('monitoringXHREnabled').set(false);
    ConsoleTestRunner.dumpConsoleMessages();
    TestRunner.completeTest();
  }
})();
