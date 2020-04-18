// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that stepping into custom element methods will lead to a pause in the callbacks.\n`);

  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');

  await TestRunner.evaluateInPagePromise(`
    function output(message) {
      if (!self._output)
          self._output = [];
      self._output.push('[page] ' + message);
    }

    function testFunction()
    {
        var proto = Object.create(HTMLElement.prototype);
        proto.createdCallback = function createdCallback()
        {
            output('Invoked createdCallback.');
        };
        proto.attachedCallback = function attachedCallback()
        {
            output('Invoked attachedCallback.');
        };
        proto.detachedCallback = function detachedCallback()
        {
            output('Invoked detachedCallback.');
        };
        proto.attributeChangedCallback = function attributeChangedCallback()
        {
            output('Invoked attributeChangedCallback.');
        };
        var FooElement = document.registerElement('x-foo', { prototype: proto });
        debugger;
        var foo = new FooElement();
        debugger;
        foo.setAttribute('a', 'b');
        debugger;
        document.body.appendChild(foo);
        debugger;
        foo.remove();
    }
  `);

  SourcesTestRunner.startDebuggerTest(step1, true);

  function step1() {
    SourcesTestRunner.runTestFunctionAndWaitUntilPaused(step2);
  }

  function checkTopFrameFunction(callFrames, expectedName) {
    var topFunctionName = callFrames[0].functionName;

    if (expectedName === topFunctionName)
      TestRunner.addResult('PASS: Did step into event listener(' + expectedName + ').');
    else
      TestRunner.addResult('FAIL: Unexpected top function: expected ' + expectedName + ', found ' + topFunctionName);
  }

  function stepOverThenIn(name, callback) {
    TestRunner.addResult('Stepping to ' + name + '...');
    SourcesTestRunner.stepOver();

    SourcesTestRunner.waitUntilResumed(SourcesTestRunner.waitUntilPaused.bind(SourcesTestRunner, function() {
      TestRunner.addResult('Stepping into ' + name + '...');
      SourcesTestRunner.stepInto();
      SourcesTestRunner.waitUntilResumed(SourcesTestRunner.waitUntilPaused.bind(SourcesTestRunner, callback));
    }));
  }

  function step2() {
    stepOverThenIn('constructor', step3);
  }

  function step3(callFrames) {
    checkTopFrameFunction(callFrames, 'createdCallback');
    SourcesTestRunner.resumeExecution(SourcesTestRunner.waitUntilPaused.bind(SourcesTestRunner, step4));
  }

  function step4() {
    stepOverThenIn('setAttribute', step5);
  }

  function step5(callFrames) {
    checkTopFrameFunction(callFrames, 'attributeChangedCallback');
    SourcesTestRunner.resumeExecution(SourcesTestRunner.waitUntilPaused.bind(SourcesTestRunner, step6));
  }

  function step6() {
    stepOverThenIn('attachedCallback', step7);
  }

  function step7(callFrames) {
    checkTopFrameFunction(callFrames, 'attachedCallback');
    SourcesTestRunner.resumeExecution(SourcesTestRunner.waitUntilPaused.bind(SourcesTestRunner, step8));
  }

  function step8() {
    stepOverThenIn('detachedCallback', step9);
  }

  function step9(callFrames) {
    checkTopFrameFunction(callFrames, 'detachedCallback');
    SourcesTestRunner.resumeExecution(step10);
  }

  async function step10() {
    const output = await TestRunner.evaluateInPageAsync('JSON.stringify(self._output)');
    TestRunner.addResults(JSON.parse(output));
    TestRunner.completeTest();
  }
})();
