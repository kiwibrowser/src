// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that custom element lifecycle events fire while debugger is paused.\n`);

  await TestRunner.loadModule('console_test_runner');
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');

  await TestRunner.evaluateInPagePromise(`
    function output(message) {
      if (!self._output)
          self._output = [];
      self._output.push('[page] ' + message);
    }
  `);

  var setup = [
    'var proto = Object.create(HTMLElement.prototype);',
    'proto.createdCallback = function() { output(\'Invoked createdCallback.\'); };',
    'proto.attachedCallback = function() { output(\'Invoked attachedCallback.\'); };',
    'proto.detachedCallback = function() { output(\'Invoked detachedCallback.\'); };',
    'proto.attributeChangedCallback = function() { output(\'Invoked attributeChangedCallback.\'); };',
    'CustomElement = document.registerElement(\'x-foo\', {prototype: proto});'
  ].join('\n');

  var lifecycleCallbacks = [
    'created = new CustomElement();', 'created.setAttribute(\'x\', \'1\');', 'document.body.appendChild(created);',
    'created.remove();'
  ].join('\n');

  SourcesTestRunner.startDebuggerTest(step1);

  function step1() {
    ConsoleTestRunner.evaluateInConsole(setup, function() {
      TestRunner.addResult('Custom element registered.');
      ConsoleTestRunner.evaluateInConsoleAndDump('new CustomElement() instanceof CustomElement', step2);
    });
  }

  function step2() {
    ConsoleTestRunner.evaluateInConsole('debugger;');
    SourcesTestRunner.waitUntilPaused(step3);
  }

  function step3() {
    ConsoleTestRunner.evaluateInConsoleAndDump('new CustomElement() instanceof CustomElement', step4);
  }

  function step4() {
    ConsoleTestRunner.evaluateInConsole(lifecycleCallbacks, step5);
  }

  function step5() {
    SourcesTestRunner.resumeExecution(step6);
  }

  async function step6() {
    const output = await TestRunner.evaluateInPageAsync('JSON.stringify(self._output)');
    TestRunner.addResults(JSON.parse(output));
    TestRunner.completeTest();
  }
})();
