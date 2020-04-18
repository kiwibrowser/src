// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(
      `Tests that style properties of elements in iframes loaded from domain different from the main document domain can be inspected. See bug 31587.\n`);
  await TestRunner.loadModule('elements_test_runner');
  await TestRunner.loadHTML(`
      <iframe src="http://localhost:8000/devtools/resources/iframe-from-different-domain-data.html" id="receiver" onload="onIFrameLoad()"></iframe>
    `);
  await TestRunner.evaluateInPagePromise(`
      var onIFrameLoadCalled = false;
      function onIFrameLoad()
      {
          if (onIFrameLoadCalled)
              return;
          onIFrameLoadCalled = true;    }
  `);

  ElementsTestRunner.selectNodeAndWaitForStyles('iframe-body', step1);

  function step1() {
    var treeItem = ElementsTestRunner.getElementStylePropertyTreeItem('background');
    ElementsTestRunner.dumpStyleTreeItem(treeItem, '');
    TestRunner.completeTest();
  }
})();
