// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that inspect() command line api works.\n`);
  await TestRunner.loadModule('console_test_runner');
  await TestRunner.loadHTML(`
      <p id="p1">
      </p>
    `);

  TestRunner.addSniffer(SDK.RuntimeModel.prototype, '_inspectRequested', sniffInspect, true);

  function sniffInspect(objectId, hints) {
    TestRunner.addResult('WebInspector.inspect called with: ' + objectId.description);
    TestRunner.addResult('WebInspector.inspect\'s hints are: ' + JSON.stringify(Object.keys(hints)));
  }

  function evalAndDump(expression, next) {
    TestRunner.addResult('\n');
    ConsoleTestRunner.evaluateInConsole(expression, dumpCallback);
    function dumpCallback(text) {
      TestRunner.addResult(expression + ' = ' + text);
      if (next)
        next();
    }
  }

  TestRunner.runTestSuite([function testRevealElement(next) {
    TestRunner.addSniffer(Common.Revealer, 'reveal', step2, true);
    evalAndDump('inspect($(\'#p1\'))');

    function step2(node, revealPromise) {
      if (!(node instanceof SDK.RemoteObject))
        return;
      revealPromise.then(step3);
    }

    function step3() {
      TestRunner.addResult('Selected node id: \'' + UI.panels.elements.selectedDOMNode().getAttribute('id') + '\'.');
      next();
    }
  }]);
})();
