// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  await TestRunner.setupStartupTest('resources/script-formatter-console.html');
  TestRunner.addResult(`Tests that the script formatting changes console line numbers.\n`);
  await TestRunner.loadModule('console_test_runner');
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');

  var panel = UI.panels.sources;
  var sourceFrame;
  var scriptFormatter;

  SourcesTestRunner.runDebuggerTestSuite([
    function testSetup(next) {
      SourcesTestRunner.scriptFormatter().then(function(sf) {
        scriptFormatter = sf;
        next();
      });
    },

    function testConsoleMessagesForFormattedScripts(next) {
      SourcesTestRunner.showScriptSource('script-formatter-console.html', didShowScriptSource);

      function didShowScriptSource(frame) {
        sourceFrame = frame;
        TestRunner.evaluateInPage('f1()', didEvaluate);
      }

      function didEvaluate() {
        dumpConsoleMessageURLs();
        TestRunner.addResult('Pre-format row message list:');
        TestRunner.addResult(JSON.stringify(sourceFrame._rowMessageBuckets.keysArray()));
        var name = panel.visibleView.uiSourceCode().name();
        scriptFormatter._toggleFormatScriptSource();
        SourcesTestRunner.showScriptSource(name + ':formatted', uiSourceCodeScriptFormatted);
      }

      function uiSourceCodeScriptFormatted() {
        dumpConsoleMessageURLs();
        TestRunner.addResult('Post-format row message list:');
        var formattedSourceFrame = panel.visibleView;
        TestRunner.addResult(JSON.stringify(formattedSourceFrame._rowMessageBuckets.keysArray()));
        next();
      }
    }
  ]);

  function dumpConsoleMessageURLs() {
    var messages = Console.ConsoleView.instance()._visibleViewMessages;
    for (var i = 0; i < messages.length; ++i) {
      var element = messages[i].toMessageElement();
      var anchor = element.querySelector('.console-message-anchor');
      TestRunner.addResult(anchor.textContent);
    }
  }
})();
