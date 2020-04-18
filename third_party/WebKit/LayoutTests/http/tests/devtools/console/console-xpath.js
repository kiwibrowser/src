// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests $x for iterator and non-iterator types.\n`);
  await TestRunner.loadModule('console_test_runner');
  await TestRunner.showPanel('console');
  await TestRunner.loadHTML(`
      <a href="http://chromium.org"></a>
      <p id="test"></p>
  `);

  TestRunner.addSniffer(
      Console.ConsoleViewMessage.prototype, '_formattedParameterAsNodeForTest', formattedParameter, true);
  ConsoleTestRunner.addConsoleViewSniffer(messageSniffer, true);

  ConsoleTestRunner.evaluateInConsole('$x(\'42\')');                           // number
  ConsoleTestRunner.evaluateInConsole('$x(\'name(/html)\')');                  // string
  ConsoleTestRunner.evaluateInConsole('$x(\'not(42)\')');                      // boolean
  ConsoleTestRunner.evaluateInConsole('$x(\'/html/body/p\').length');          // node iterator
  ConsoleTestRunner.evaluateInConsole('$x(\'//a/@href\')[0]');                 // href, should not throw
  ConsoleTestRunner.evaluateInConsole('$x(\'./a/@href\', document.body)[0]');  // relative to document.body selector
  ConsoleTestRunner.evaluateInConsole('$x(\'./a@href\', document.body)');      // incorrect selector, shouldn't crash
  TestRunner.evaluateInPage('console.log(\'complete\')');                      // node iterator

  var completeMessageReceived = false;
  function messageSniffer(uiMessage) {
    if (uiMessage.element().deepTextContent().indexOf('complete') !== -1) {
      completeMessageReceived = true;
      maybeCompleteTest();
    }
  }

  var waitForParameteres = 2;
  function formattedParameter() {
    waitForParameteres--;
    maybeCompleteTest();
  }

  function maybeCompleteTest() {
    if (!waitForParameteres && completeMessageReceived) {
      ConsoleTestRunner.dumpConsoleMessages();
      TestRunner.completeTest();
    }
  }
})();
