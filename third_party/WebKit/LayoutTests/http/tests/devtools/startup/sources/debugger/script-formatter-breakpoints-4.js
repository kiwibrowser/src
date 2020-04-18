// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  await TestRunner.setupStartupTest('resources/script-formatter-breakpoints-4.html');
  TestRunner.addResult(`Tests the script formatting is working fine with breakpoints.\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');

  Bindings.breakpointManager._storage._breakpoints = new Map();
  var panel = UI.panels.sources;
  var scriptFormatter;

  SourcesTestRunner.runDebuggerTestSuite([
    function testSetup(next) {
      SourcesTestRunner.scriptFormatter().then(function(sf) {
        scriptFormatter = sf;
        next();
      });
    },

    function testBreakpointSetInOriginalAndRemovedInFormatted(next) {
      SourcesTestRunner.showScriptSource('script-formatter-breakpoints-4.html', didShowScriptSource);

      function didShowScriptSource(sourceFrame) {
        TestRunner.addResult('Adding breakpoint.');
        TestRunner.addSniffer(
            Bindings.BreakpointManager.ModelBreakpoint.prototype,
            '_addResolvedLocation', breakpointResolved);
        SourcesTestRunner.setBreakpoint(sourceFrame, 9, '', true);
      }

      function breakpointResolved() {
        TestRunner.addResult('Formatting.');
        TestRunner.addSniffer(
            Sources.ScriptFormatterEditorAction.prototype, '_updateButton', uiSourceCodeScriptFormatted);
        scriptFormatter._toggleFormatScriptSource();
      }

      async function uiSourceCodeScriptFormatted() {
        TestRunner.addResult('Removing breakpoint.');
        var formattedSourceFrame = panel.visibleView;
        await SourcesTestRunner.waitUntilDebuggerPluginLoaded(
            formattedSourceFrame);
        SourcesTestRunner.removeBreakpoint(formattedSourceFrame, 11);
        TestRunner.addResult('Unformatting.');
        Sources.sourceFormatter.discardFormattedUISourceCode(panel.visibleView.uiSourceCode());
        var breakpoints = Bindings.breakpointManager._storage._setting.get();
        TestRunner.assertEquals(breakpoints.length, 0, 'There should not be any breakpoints in the storage.');
        next();
      }
    }
  ]);
})();
