// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that elements panel shows DOM tree structure.\n`);
  await TestRunner.loadModule('elements_test_runner');
  await TestRunner.showPanel('elements');
  await TestRunner.loadHTML(`
      <!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
      <div id="level1">
          <div id="level2">&quot;Quoted Text&quot;. Special&#xA0;characters: &gt;&lt;&quot;'&#xA0;&#x2002;&#x2003;&#x2009;&#x200A;&#x200B;&#x200C;&#x200D;&#x200E;&#x200F; &#x202A;&#x202B;&#x202C;&#x202D;&#x202E;&#xAD;<div id="level3"></div>
          </div>
      </div>
      <div id="control-character"></div>
    `);
  await TestRunner.evaluateInPagePromise(`
      document.querySelector("#control-character").textContent = "\ufeff\u0093";
  `);

  // Warm up highlighter module.
  runtime.loadModulePromise('source_frame').then(function() {
    ElementsTestRunner.expandElementsTree(step1);
  });

  function step1() {
    ElementsTestRunner.dumpElementsTree();
    TestRunner.completeTest();
  }
})();
