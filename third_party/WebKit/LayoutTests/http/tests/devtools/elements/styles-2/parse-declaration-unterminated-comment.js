// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that CSSParser correctly parses declarations with unterminated comments.\n`);
  await TestRunner.loadModule('elements_test_runner');
  await TestRunner.showPanel('elements');
  await TestRunner.loadHTML(`
      <div id="inspected1" style="color: red /* foo: bar;"></div>
      <div id="inspected2" style="color: green; /* foo: bar;"></div>
    `);

  ElementsTestRunner.selectNodeAndWaitForStyles('inspected1', dumpStylesFirst);

  function dumpStylesFirst() {
    ElementsTestRunner.dumpSelectedElementStyles(true);
    ElementsTestRunner.selectNodeAndWaitForStyles('inspected2', dumpStylesSecond);
  }

  function dumpStylesSecond() {
    ElementsTestRunner.dumpSelectedElementStyles(true);
    TestRunner.completeTest();
  }
})();
