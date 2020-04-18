// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  await TestRunner.setupStartupTest('resources/console-format-startup.html');

  TestRunner.addResult('Tests console logging for messages that happen before DevTools is open.\n');

  await TestRunner.loadModule('console_test_runner');
  await TestRunner.showPanel('console');

  TestRunner.hideInspectorView();
  var total = await TestRunner.evaluateInPageRemoteObject('globals.length');
  loopOverGlobals(0, total);
  function loopOverGlobals(current, total) {
    function advance() {
      var next = current + 1;
      if (next == total.description)
        ConsoleTestRunner.waitForRemoteObjectsConsoleMessages(onRemoteObjectsLoaded);
      else
        loopOverGlobals(next, total);
    }

    function onRemoteObjectsLoaded() {
      ConsoleTestRunner.dumpConsoleMessagesIgnoreErrorStackFrames();
      TestRunner.addResult('Expanded all messages');
      ConsoleTestRunner.expandConsoleMessages(
          ConsoleTestRunner.expandConsoleMessagesErrorParameters.bind(this, finish), undefined, function(section) {
            return section.element.firstChild.textContent !== '#text';
          });
    }

    function finish() {
      ConsoleTestRunner.dumpConsoleMessagesIgnoreErrorStackFrames();
      TestRunner.completeTest();
    }

    TestRunner.evaluateInPage('log(' + current + ')');
    TestRunner.deprecatedRunAfterPendingDispatches(evalInConsole);
    function evalInConsole() {
      ConsoleTestRunner.evaluateInConsole('globals[' + current + ']');
      TestRunner.deprecatedRunAfterPendingDispatches(advance);
    }
  }
})();