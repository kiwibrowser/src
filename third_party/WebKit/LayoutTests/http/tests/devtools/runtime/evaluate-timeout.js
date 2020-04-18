// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  await TestRunner.loadModule('sources_test_runner');
  TestRunner.addResult("Test frontend's timeout support.\n");

  const executionContext = UI.context.flavor(SDK.ExecutionContext);
  const regularExpression = '1 + 1';
  const infiniteExpression = 'while (1){}';

  await runtimeTestCase(regularExpression);
  await runtimeTestCase(infiniteExpression);
  await runtimeTestCase(regularExpression);

  let supports = executionContext.runtimeModel.hasSideEffectSupport();
  TestRunner.addResult(`\nDoes the runtime also support side effect checks? ${supports}`);
  TestRunner.addResult(`\nClearing cached side effect support`);
  executionContext.runtimeModel._hasSideEffectSupport = null;

  // Debugger evaluateOnCallFrame test.
  await TestRunner.evaluateInPagePromise(`
    function testFunction()
    {
        debugger;
    }
  `);

  await SourcesTestRunner.startDebuggerTestPromise();
  await SourcesTestRunner.runTestFunctionAndWaitUntilPausedPromise();

  await debuggerTestCase(regularExpression);
  await debuggerTestCase(infiniteExpression);
  await debuggerTestCase(regularExpression);

  supports = executionContext.runtimeModel.hasSideEffectSupport();
  TestRunner.addResult(`Does the runtime also support side effect checks? ${supports}`);

  SourcesTestRunner.completeDebuggerTest();

  async function runtimeTestCase(expression) {
    TestRunner.addResult(`\nTesting expression ${expression} with timeout: 0`);
    const result = await executionContext.evaluate({expression, timeout: 0});
    printDetails(result);
  }

  async function debuggerTestCase(expression) {
    TestRunner.addResult(`\nTesting expression ${expression} with timeout: 0`);
    const result = await executionContext.debuggerModel.selectedCallFrame().evaluate({expression, timeout: 0});
    printDetails(result);
  }

  function printDetails(result) {
    const customFormatters = {};
    for (let name of ['_runtimeModel', '_runtimeAgent'])
      customFormatters[name] = 'formatAsTypeNameOrNull';
    TestRunner.dump(result, customFormatters);
  }
})();
