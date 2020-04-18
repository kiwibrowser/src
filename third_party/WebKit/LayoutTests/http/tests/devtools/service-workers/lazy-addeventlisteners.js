// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(
      `Tests that a warning is shown in the console if addEventListener is called after initial evaluation of the service worker script.\n`);
  await TestRunner.loadModule('console_test_runner');
  await TestRunner.loadModule('application_test_runner');
    // Note: every test that uses a storage API must manually clean-up state from previous tests.
  await ApplicationTestRunner.resetState();

  await TestRunner.showPanel('resources');

  var scriptURL = 'http://127.0.0.1:8000/devtools/service-workers/resources/service-worker-lazy-addeventlistener.js';
  var scope = 'http://127.0.0.1:8000/devtools/service-workers/resources/lazy-scope';
  ConsoleTestRunner.waitForConsoleMessages(1, () => {
    ConsoleTestRunner.dumpConsoleMessages();
    TestRunner.completeTest();
  });
  ApplicationTestRunner.registerServiceWorker(scriptURL, scope);
})();
