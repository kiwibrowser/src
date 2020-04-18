// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that overriding Function.prototype.bind does not break inspector.\n`);

  await TestRunner.loadModule('console_test_runner');
  await TestRunner.showPanel('console');

  await TestRunner.evaluateInPagePromise(`
     var foo = 'fooValue';
    Function.prototype.bind = function () { throw ":P"; };
  `);

  ConsoleTestRunner.evaluateInConsole('foo', step1);

  function step1() {
    ConsoleTestRunner.dumpConsoleMessages();
    TestRunner.completeTest();
  }
})();
