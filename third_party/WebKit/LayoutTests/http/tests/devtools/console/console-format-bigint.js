// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// TODO(luoe): once BigInts are on by default, merge this test into
// http/tests/devtools/console/console-format.js

(async function() {
  TestRunner.addResult('Tests that console properly displays BigInts.\n');

  await TestRunner.loadModule('console_test_runner');
  await TestRunner.showPanel('console');

  await TestRunner.evaluateInPagePromise(`
    var wrappedBigInt = Object(BigInt(2));
    console.log(1n);
    console.log(wrappedBigInt);
    console.log([1n]);
    console.log([wrappedBigInt]);
    console.log(null, 1n, wrappedBigInt);
  `);

  ConsoleTestRunner.dumpConsoleMessages(false, false, TestRunner.textContentWithLineBreaks);
  TestRunner.addResult('Expanded all messages');
  ConsoleTestRunner.expandConsoleMessages(dumpConsoleMessages);

  function dumpConsoleMessages() {
    ConsoleTestRunner.dumpConsoleMessages(false, false, TestRunner.textContentWithLineBreaks);
    TestRunner.completeTest();
  }
})();
