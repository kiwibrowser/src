// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that setting selector text can be undone.\n`);
  await TestRunner.loadModule('elements_test_runner');
  await TestRunner.showPanel('elements');
  await TestRunner.loadHTML(`
      <style>
      #inspected {
        color: green;
      }
      </style>
      <div id="inspected"></div>
      <div id="other"></div>
    `);

  ElementsTestRunner.selectNodeAndWaitForStyles('inspected', step1);

  function step1() {
    TestRunner.addResult('=== Before selector modification ===');
    ElementsTestRunner.dumpSelectedElementStyles(true);
    var section = ElementsTestRunner.firstMatchedStyleSection();
    section.startEditingSelector();
    section._selectorElement.textContent = '#inspected, #other';
    section._selectorElement.dispatchEvent(TestRunner.createKeyEvent('Enter'));
    ElementsTestRunner.selectNodeAndWaitForStyles('other', step2);
  }

  function step2() {
    TestRunner.addResult('=== After selector modification ===');
    ElementsTestRunner.dumpSelectedElementStyles(true);
    SDK.domModelUndoStack.undo();
    ElementsTestRunner.selectNodeAndWaitForStyles('inspected', step3);
  }

  function step3() {
    TestRunner.addResult('=== After undo ===');
    ElementsTestRunner.dumpSelectedElementStyles(true);

    SDK.domModelUndoStack.redo();
    ElementsTestRunner.selectNodeAndWaitForStyles('other', step4);
  }

  function step4() {
    TestRunner.addResult('=== After redo ===');
    ElementsTestRunner.dumpSelectedElementStyles(true);
    TestRunner.completeTest();
  }
})();
