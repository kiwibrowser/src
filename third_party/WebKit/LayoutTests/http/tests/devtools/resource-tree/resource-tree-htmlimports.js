// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests resource tree model for imports.\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.loadModule('application_test_runner');
  await TestRunner.showPanel('resources');
  await TestRunner.loadHTML(`
    <link rel="import" href="resources/import-hello.html">
    <!-- import-hello.html shouldn't be shown twice as it shares same resource as above -->
    <link rel="import" href="resources/import-hello.html">
  `);

  await Promise.all([
    TestRunner.waitForUISourceCode('import-hello.html'),
    TestRunner.waitForUISourceCode('import-child.html'),
    TestRunner.waitForUISourceCode('import-hello.js'),
  ]);

  ApplicationTestRunner.dumpResourceTreeEverything();
  TestRunner.completeTest();
})();
