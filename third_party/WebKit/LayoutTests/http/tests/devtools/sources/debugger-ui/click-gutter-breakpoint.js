// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that breakpoints can be added and removed by clicking the gutter.\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');
  await TestRunner.addScriptTag('../debugger/resources/click-breakpoints.js');

  function clickCodeMirrorLineNumber(lineNumber, isRemove, sourceFrame) {
    var element = Array.from(document.getElementsByClassName('CodeMirror-linenumber'))
                      .filter(x => x.textContent === (lineNumber + 1).toString())[0];
    if (!element) {
      TestRunner.addResult('CodeMirror Gutter Not Found:' + lineNumber);
      SourcesTestRunner.completeDebuggerTest();
      return Promise.resolve();
    }
    var rect = element.getBoundingClientRect();
    eventSender.mouseMoveTo(rect.left + rect.width / 2, rect.top + rect.height / 2);
    eventSender.mouseDown();
    eventSender.mouseUp();
    if (!isRemove)
      return new Promise(
          resolve => TestRunner.addSniffer(
              Sources.DebuggerPlugin.prototype, '_breakpointWasSetForTest',
              resolve, false));
    return Promise.resolve();
  }

  Bindings.breakpointManager._storage._breakpoints = new Map();
  var panel = UI.panels.sources;
  var scriptFormatter;
  var formattedSourceFrame;

  SourcesTestRunner.startDebuggerTest(
      () => SourcesTestRunner.showScriptSource('click-breakpoints.js', didShowScriptSource));


  function didShowScriptSource(sourceFrame) {
    clickCodeMirrorLineNumber(2, false, sourceFrame)
        .then(() => clickCodeMirrorLineNumber(2, true, sourceFrame))
        .then(() => clickCodeMirrorLineNumber(3, false, sourceFrame))
        .then(runScript);
  }

  function runScript() {
    Promise
        .all([
          SourcesTestRunner.waitBreakpointSidebarPane(),
          new Promise(resolve => SourcesTestRunner.waitUntilPaused(resolve))
        ])
        .then(() => SourcesTestRunner.dumpBreakpointSidebarPane('while paused'))
        .then(() => SourcesTestRunner.completeDebuggerTest());
    TestRunner.evaluateInPageWithTimeout('f2()');
  }
})();
