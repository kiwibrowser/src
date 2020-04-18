// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(
      `Tests that disabling shorthand removes the "overriden" mark from the UA shorthand it overrides.\n`);
  await TestRunner.loadModule('elements_test_runner');
  await TestRunner.showPanel('elements');
  await TestRunner.loadHTML(`
      <body id="body-id" style="margin: 10px">
    `);

  ElementsTestRunner.selectNodeAndWaitForStyles('body-id', step1);

  function step1() {
    TestRunner.addResult('Before disable');
    ElementsTestRunner.dumpSelectedElementStyles(true, false, true);
    ElementsTestRunner.toggleStyleProperty('margin', false);
    ElementsTestRunner.waitForStyles('body-id', step2);
  }

  function step2() {
    TestRunner.addResult('After disable');
    ElementsTestRunner.dumpSelectedElementStyles(true, false, true);
    ElementsTestRunner.toggleStyleProperty('margin', true);
    ElementsTestRunner.waitForStyles('body-id', step3);
  }

  function step3() {
    TestRunner.addResult('After enable');
    ElementsTestRunner.dumpSelectedElementStyles(true, false, true);
    TestRunner.completeTest();
  }
})();
