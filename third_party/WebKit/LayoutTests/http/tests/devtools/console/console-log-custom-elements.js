// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that logging custom elements uses proper formatting.\n`);

  await TestRunner.loadModule('console_test_runner');
  await TestRunner.showPanel('console');
  await TestRunner.loadHTML(`
    <foo-bar></foo-bar>
    <foo-bar2></foo-bar2>
  `);
  await TestRunner.evaluateInPagePromise(`
    function registerNonElement()
    {
      var nonElementProto = {
        createdCallback: function()
        {
          console.dir(this);
        }
      };
      var nonElementOptions = { prototype: nonElementProto };
      document.registerElement("foo-bar", nonElementOptions);
    }

    function registerElement()
    {
      var elementProto = Object.create(HTMLElement.prototype);
      elementProto.createdCallback = function()
      {
        console.dir(this);
      };
      var elementOptions = { prototype: elementProto };
      document.registerElement("foo-bar2", elementOptions);
    }
  `);

  ConsoleTestRunner.waitUntilMessageReceived(step1);
  TestRunner.evaluateInPage('registerNonElement();');

  function step1() {
    ConsoleTestRunner.waitUntilMessageReceived(step2);
    TestRunner.evaluateInPage('registerElement();');
  }

  function step2() {
    ConsoleTestRunner.dumpConsoleMessages();
    TestRunner.completeTest();
  }
})();
