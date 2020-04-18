// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(
      `Tests that rules from imported stylesheets are correctly shown and are editable in inspector.\n`);
  await TestRunner.loadModule('elements_test_runner');
  await TestRunner.showPanel('elements');
  await TestRunner.loadHTML(`
      <div id="square" class="square"></div>
    `);
  await TestRunner.addHTMLImport('../styles/resources/imported-stylesheet.html');

  ElementsTestRunner.selectNodeAndWaitForStyles('square', step1);

  function step1() {
    TestRunner.addResult('Rules before toggling:');
    ElementsTestRunner.dumpSelectedElementStyles(true, false, true);
    ElementsTestRunner.waitForStyleApplied(step2);
    ElementsTestRunner.toggleMatchedStyleProperty('background-color', false);
  }

  function step2() {
    TestRunner.addResult('Rules after toggling:');
    ElementsTestRunner.dumpSelectedElementStyles(true, false, true);
    TestRunner.completeTest();
  }
})();
