// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that adding a property is undone properly.\n`);
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

  ElementsTestRunner.selectNodeAndWaitForStyles('container', testAppendProperty);

  function testAppendProperty() {
    TestRunner.addResult('=== Last property ===');
    testAddProperty('margin-left: 2px', undefined, testInsertBegin);
  }

  function testInsertBegin() {
    TestRunner.addResult('=== First property ===');
    testAddProperty('margin-top: 0px', 0, testInsertMiddle);
  }

  function testInsertMiddle() {
    TestRunner.addResult('=== Middle property ===');
    testAddProperty('margin-right: 1px', 1, TestRunner.completeTest.bind(TestRunner));
  }

  function testAddProperty(propertyText, index, callback) {
    TestRunner.addResult('(Initial value)');
    ElementsTestRunner.dumpSelectedElementStyles(true);

    var treeItem = ElementsTestRunner.getMatchedStylePropertyTreeItem('font-weight');
    var treeElement = treeItem.section().addNewBlankProperty(index);
    treeElement.applyStyleText(propertyText, true);
    ElementsTestRunner.waitForStyles('container', step1);

    function step1() {
      TestRunner.addResult('(After adding property)');
      ElementsTestRunner.dumpSelectedElementStyles(true);

      SDK.domModelUndoStack.undo();
      ElementsTestRunner.selectNodeAndWaitForStyles('other', step2);
    }

    function step2() {
      TestRunner.addResult('(After undo)');
      ElementsTestRunner.dumpSelectedElementStyles(true);

      SDK.domModelUndoStack.redo();
      ElementsTestRunner.selectNodeAndWaitForStyles('container', step3);
    }

    function step3() {
      TestRunner.addResult('(After redo)');
      ElementsTestRunner.dumpSelectedElementStyles(true);
      callback();
    }
  }
})();
