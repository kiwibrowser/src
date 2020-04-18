// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  await TestRunner.setupStartupTest('resources/script-formatter-breakpoints-1.html');
  TestRunner.addResult(`Tests the script formatting is working fine with breakpoints.\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');

  Bindings.breakpointManager._storage._breakpoints = new Map();
  var panel = UI.panels.sources;
  var scriptFormatter;
  var sourceFrame;

  SourcesTestRunner.runDebuggerTestSuite([
    function testSetup(next) {
      SourcesTestRunner.scriptFormatter().then(function(sf) {
        scriptFormatter = sf;
        next();
      });
    },

    function testBreakpointsInOriginalAndFormattedSource(next) {
      SourcesTestRunner.showScriptSource('script-formatter-breakpoints-1.html', didShowScriptSource);

      function didShowScriptSource(frame) {
        sourceFrame = frame;
        SourcesTestRunner.setBreakpoint(sourceFrame, 9, '', true);
        Promise.all([SourcesTestRunner.waitBreakpointSidebarPane(true), SourcesTestRunner.waitUntilPausedPromise()])
            .then(pausedInF1);
        TestRunner.evaluateInPageWithTimeout('f1()');
      }

      function pausedInF1(callFrames) {
        SourcesTestRunner.dumpBreakpointSidebarPane('while paused in raw');
        SourcesTestRunner.resumeExecution(resumed);
      }

      function resumed() {
        TestRunner.addSniffer(
            Sources.ScriptFormatterEditorAction.prototype, '_updateButton', uiSourceCodeScriptFormatted);
        scriptFormatter._toggleFormatScriptSource();
      }

      function uiSourceCodeScriptFormatted() {
        // There should be a breakpoint in f1 although script is pretty-printed.
        Promise.all([SourcesTestRunner.waitBreakpointSidebarPane(true), SourcesTestRunner.waitUntilPausedPromise()])
            .then(pausedInF1Again);
        TestRunner.evaluateInPageWithTimeout('f1()');
      }

      function pausedInF1Again(callFrames) {
        SourcesTestRunner.dumpBreakpointSidebarPane('while paused in pretty printed');
        Sources.sourceFormatter.discardFormattedUISourceCode(panel.visibleView.uiSourceCode());
        SourcesTestRunner.waitBreakpointSidebarPane().then(onBreakpointsUpdated);
      }

      function onBreakpointsUpdated() {
        SourcesTestRunner.dumpBreakpointSidebarPane('while paused in raw');
        SourcesTestRunner.removeBreakpoint(sourceFrame, 9);
        SourcesTestRunner.resumeExecution(next);
      }
    }
  ]);
})();
