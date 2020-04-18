// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(
      `Tests that the "style" attribute removal results in the Styles sidebar pane update (not a crash). Bug 51478\n`);
  await TestRunner.loadModule('elements_test_runner');
  await TestRunner.showPanel('elements');
  await TestRunner.loadHTML(`
      <div id="inspected" style="color: red"></div>
    `);

  ElementsTestRunner.selectNodeAndWaitForStyles('inspected', step1);

  function step1(node) {
    TestRunner.addResult('Before style property removal:');
    ElementsTestRunner.dumpSelectedElementStyles(true);
    node.removeAttribute('style');
    ElementsTestRunner.waitForStyles('inspected', step2);
  }

  function step2() {
    TestRunner.addResult('After style property removal:');
    ElementsTestRunner.dumpSelectedElementStyles(true);
    TestRunner.completeTest();
  }
})();
