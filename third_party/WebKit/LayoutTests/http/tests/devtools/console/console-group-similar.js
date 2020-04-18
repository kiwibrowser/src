// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that console correctly groups similar messages.\n`);

  await TestRunner.loadModule('console_test_runner');
  await TestRunner.showPanel('console');

  // Show all messages, including verbose.
  Console.ConsoleView.instance()._filter._currentFilter.levelsMask = Console.ConsoleFilter.allLevelsFilterValue();

  for (var i = 0; i < 5; i++) {
    addViolationMessage('Verbose-level violation', `script${i}.js`, SDK.ConsoleMessage.MessageLevel.Verbose);
    addViolationMessage('Error-level violation', `script${i}.js`, SDK.ConsoleMessage.MessageLevel.Error);
    addConsoleAPIMessage('ConsoleAPI log', `script${i}.js`);
    await ConsoleTestRunner.evaluateInConsolePromise(`'evaluated command'`);
    await TestRunner.evaluateInPagePromise(`Promise.reject()`);
  }

  ConsoleTestRunner.dumpConsoleMessages();

  TestRunner.addResult('\n\nStop grouping messages:\n');
  Console.ConsoleView.instance()._groupSimilarSetting.set(false);
  ConsoleTestRunner.dumpConsoleMessages();
  TestRunner.completeTest();

  /**
   * @param {string} text
   * @param {string} url
   * @param {string} level
   */
  function addViolationMessage(text, url, level) {
    var message = new SDK.ConsoleMessage(
        null, SDK.ConsoleMessage.MessageSource.Violation, level,
        text, SDK.ConsoleMessage.MessageType.Log, url);
    SDK.consoleModel.addMessage(message);
  }

  /**
   * @param {string} text
   * @param {string} url
   */
  function addConsoleAPIMessage(text,  url) {
    var message = new SDK.ConsoleMessage(
        null, SDK.ConsoleMessage.MessageSource.ConsoleAPI, SDK.ConsoleMessage.MessageLevel.Info,
        text, SDK.ConsoleMessage.MessageType.Log, url);
    SDK.consoleModel.addMessage(message);
  }
})();
