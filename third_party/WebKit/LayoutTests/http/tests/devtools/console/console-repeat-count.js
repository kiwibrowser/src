// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that repeat count is properly updated.\n`);

  await TestRunner.loadModule('console_test_runner');
  await TestRunner.showPanel('console');
  await TestRunner.evaluateInPagePromise(`

    function dumpMessages()
    {
      for (var i = 0; i < 2; ++i)
        console.log("Message");

      for (var i = 0; i < 2; ++i)
        console.log(new Error("Message with error"));

      for (var i = 0; i < 2; ++i)
        console.error({a: 1});
    }

    function throwObjects() {
      for (var i = 0; i < 2; ++i)
        setTimeout(() => { throw {a: 1}; }, 0);
    }

    function throwPrimitiveValues() {
      for (var i = 0; i < 2; ++i)
        setTimeout(() => { throw "Primitive value"; }, 0);
    }
    //# sourceURL=console-repeat-count.js
  `);

  await TestRunner.evaluateInPagePromise('dumpMessages()');
  await TestRunner.evaluateInPagePromise('throwPrimitiveValues()');
  await TestRunner.evaluateInPagePromise('throwObjects()');
  ConsoleTestRunner.waitForConsoleMessages(7, () => {
    ConsoleTestRunner.dumpConsoleMessages();
    TestRunner.completeTest();
  });
})();
