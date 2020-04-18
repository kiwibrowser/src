// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Checks that JavaScriptSourceFrame show breakpoints correctly\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');
  await TestRunner.addScriptTag('resources/edit-me-breakpoints.js');

  function waitAndDumpDecorations(sourceFrame) {
    return SourcesTestRunner.waitBreakpointSidebarPane(true).then(
        () => SourcesTestRunner.dumpDebuggerPluginBreakpoints(sourceFrame));
  }

  Bindings.breakpointManager._storage._breakpoints = new Map();
  SourcesTestRunner.runDebuggerTestSuite([
    function testAddRemoveBreakpoint(next) {
      var javaScriptSourceFrame;
      SourcesTestRunner.showScriptSource('edit-me-breakpoints.js', addBreakpoint);

      function addBreakpoint(sourceFrame) {
        javaScriptSourceFrame = sourceFrame;
        TestRunner.addResult('Setting breakpoint');
        SourcesTestRunner.createNewBreakpoint(javaScriptSourceFrame, 2, '', true)
            .then(() => waitAndDumpDecorations(javaScriptSourceFrame))
            .then(removeBreakpoint);
      }

      function removeBreakpoint() {
        TestRunner.addResult('Toggle breakpoint');
        SourcesTestRunner.toggleBreakpoint(javaScriptSourceFrame, 2);
        waitAndDumpDecorations(javaScriptSourceFrame).then(() => next());
      }
    },

    function testTwoBreakpointsResolvedInOneLine(next) {
      var javaScriptSourceFrame;
      SourcesTestRunner.showScriptSource('edit-me-breakpoints.js', addBreakpoint);

      function addBreakpoint(sourceFrame) {
        javaScriptSourceFrame = sourceFrame;
        TestRunner.addResult('Setting breakpoint');
        SourcesTestRunner.createNewBreakpoint(javaScriptSourceFrame, 2, '', true)
            .then(() => SourcesTestRunner.createNewBreakpoint(javaScriptSourceFrame, 0, 'true', true))
            .then(() => waitAndDumpDecorations(javaScriptSourceFrame))
            .then(removeBreakpoint);
      }

      function removeBreakpoint() {
        TestRunner.addResult('Toggle breakpoint');
        SourcesTestRunner.toggleBreakpoint(javaScriptSourceFrame, 2);
        waitAndDumpDecorations(javaScriptSourceFrame).then(() => next());
      }
    },

    function testDecorationInGutter(next) {
      var javaScriptSourceFrame;
      SourcesTestRunner.showScriptSource('edit-me-breakpoints.js', addRegularDisabled);

      function addRegularDisabled(sourceFrame) {
        javaScriptSourceFrame = sourceFrame;
        TestRunner.addResult('Adding regular disabled breakpoint');
        waitAndDumpDecorations(javaScriptSourceFrame).then(addConditionalDisabled);
        SourcesTestRunner.createNewBreakpoint(javaScriptSourceFrame, 2, '', false);
      }

      function addConditionalDisabled() {
        TestRunner.addResult('Adding conditional disabled breakpoint');
        waitAndDumpDecorations(javaScriptSourceFrame).then(addRegularEnabled);
        SourcesTestRunner.createNewBreakpoint(javaScriptSourceFrame, 0, 'true', false);
      }

      function addRegularEnabled() {
        TestRunner.addResult('Adding regular enabled breakpoint');
        waitAndDumpDecorations(javaScriptSourceFrame).then(addConditionalEnabled);
        SourcesTestRunner.createNewBreakpoint(javaScriptSourceFrame, 0, '', true);
      }

      function addConditionalEnabled() {
        TestRunner.addResult('Adding conditional enabled breakpoint');
        waitAndDumpDecorations(javaScriptSourceFrame).then(disableAll);
        SourcesTestRunner.createNewBreakpoint(javaScriptSourceFrame, 0, 'true', true);
      }

      function disableAll() {
        TestRunner.addResult('Disable breakpoints');
        waitAndDumpDecorations(javaScriptSourceFrame).then(enabledAll);
        SourcesTestRunner.toggleBreakpoint(javaScriptSourceFrame, 2, true);
      }

      function enabledAll() {
        TestRunner.addResult('Enable breakpoints');
        waitAndDumpDecorations(javaScriptSourceFrame).then(removeAll);
        SourcesTestRunner.toggleBreakpoint(javaScriptSourceFrame, 2, true);
      }

      function removeAll() {
        TestRunner.addResult('Remove breakpoints');
        waitAndDumpDecorations(javaScriptSourceFrame).then(next);
        SourcesTestRunner.toggleBreakpoint(javaScriptSourceFrame, 2, false);
      }
    }
  ]);
})();
