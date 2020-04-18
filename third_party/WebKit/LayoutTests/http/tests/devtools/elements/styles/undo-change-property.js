// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that changing a property is undone properly.\n`);
  await TestRunner.loadModule('elements_test_runner');
  await TestRunner.showPanel('elements');
  await TestRunner.loadHTML(`
      <style>
      .container {
        font-weight: bold
      }
      </style>
      <div id="container" class="container"></div>
      <div id="other" class="container"></div>
    `);

  ElementsTestRunner.selectNodeAndWaitForStyles('container', step1);

  function step1() {
    TestRunner.addResult('Initial value');
    ElementsTestRunner.dumpSelectedElementStyles(true);

    var treeItem = ElementsTestRunner.getMatchedStylePropertyTreeItem('font-weight');
    treeItem.applyStyleText('font-weight: normal', false);
    ElementsTestRunner.waitForStyles('container', step2);
  }

  function step2() {
    TestRunner.addResult('After changing property');
    ElementsTestRunner.dumpSelectedElementStyles(true);

    SDK.domModelUndoStack.undo();
    ElementsTestRunner.selectNodeAndWaitForStyles('other', step3);
  }

  function step3() {
    TestRunner.addResult('After undo');
    ElementsTestRunner.dumpSelectedElementStyles(true);

    SDK.domModelUndoStack.redo();
    ElementsTestRunner.selectNodeAndWaitForStyles('container', step4);
  }

  function step4() {
    TestRunner.addResult('After redo');
    ElementsTestRunner.dumpSelectedElementStyles(true);
    TestRunner.completeTest();
  }
})();
