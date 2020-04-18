// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that removal of property following its disabling works.\n`);
  await TestRunner.loadModule('elements_test_runner');
  await TestRunner.showPanel('elements');
  await TestRunner.loadHTML(`
      <div id="container" style="font-weight:bold">
      </div>

      <div id="other">
      </div>
    `);

  ElementsTestRunner.selectNodeAndWaitForStyles('container', step1);

  function step1() {
    // Disable property
    TestRunner.addResult('Before disable');
    ElementsTestRunner.dumpSelectedElementStyles(true, true);
    ElementsTestRunner.toggleStyleProperty('font-weight', false);
    ElementsTestRunner.waitForStyleApplied(step2);
  }

  function step2() {
    // Delete style
    TestRunner.addResult('After disable');
    ElementsTestRunner.dumpSelectedElementStyles(true, true);

    var treeItem = ElementsTestRunner.getElementStylePropertyTreeItem('font-weight');
    treeItem.applyStyleText('', false);

    ElementsTestRunner.waitForStyleApplied(step3);
  }

  function step3() {
    ElementsTestRunner.selectNodeWithId('other', step4);
  }

  function step4() {
    ElementsTestRunner.selectNodeAndWaitForStyles('container', step5);
  }

  function step5(node) {
    TestRunner.addResult('After delete');
    ElementsTestRunner.dumpSelectedElementStyles(true, true);
    TestRunner.completeTest();
  }
})();
