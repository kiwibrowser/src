// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that changing selected node while editing style does update styles sidebar.\n`);
  await TestRunner.loadModule('elements_test_runner');
  await TestRunner.showPanel('elements');
  await TestRunner.loadHTML(`
      <div id="inspected" style="color: red">Text</div>
      <div id="other" style="color: blue"></div>
    `);

  ElementsTestRunner.selectNodeAndWaitForStyles('inspected', step1);

  var treeElement;
  var section;

  function step1() {
    ElementsTestRunner.dumpSelectedElementStyles(true, true, true);
    treeElement = ElementsTestRunner.getElementStylePropertyTreeItem('color');

    treeElement.startEditing();
    treeElement.nameElement.textContent = 'background';

    ElementsTestRunner.selectNodeAndWaitForStyles('other', step2);
  }

  function step2() {
    ElementsTestRunner.dumpSelectedElementStyles(true, true, true);
    ElementsTestRunner.selectNodeAndWaitForStyles('inspected', step3);
  }

  function step3() {
    ElementsTestRunner.dumpSelectedElementStyles(true, true, true);
    TestRunner.completeTest();
  }
})();
