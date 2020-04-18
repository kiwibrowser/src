// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(
      `Tests that breadcrumbs are updated upon involved element's attribute changes in the Elements panel.\n`);
  await TestRunner.loadModule('elements_test_runner');
  await TestRunner.showPanel('elements');
  await TestRunner.loadHTML(`
      <!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
      <div class="firstClass">
          <div id="target"></div>
      </div>
    `);
  await TestRunner.evaluateInPagePromise(`
      function changeClass()
      {
          document.getElementsByClassName("firstClass")[0].className = "anotherClass";
      }

      function deleteClass()
      {
          document.getElementsByClassName("anotherClass")[0].className = "";
      }
  `);

  ElementsTestRunner.expandElementsTree(step0);

  function step0() {
    TestRunner.addSniffer(Elements.ElementsBreadcrumbs.prototype, 'update', step1);
    ElementsTestRunner.selectNodeWithId('target');
  }

  function step1() {
    ElementsTestRunner.dumpBreadcrumb('Original breadcrumb');
    TestRunner.addSniffer(Elements.ElementsBreadcrumbs.prototype, 'update', step2);
    TestRunner.evaluateInPage('changeClass()');
  }

  function step2() {
    ElementsTestRunner.dumpBreadcrumb('After class change');
    TestRunner.addSniffer(Elements.ElementsBreadcrumbs.prototype, 'update', step3);
    TestRunner.evaluateInPage('deleteClass()');
  }

  function step3() {
    ElementsTestRunner.dumpBreadcrumb('After class removal');
    TestRunner.completeTest();
  }
})();
