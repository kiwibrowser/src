// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Checks that JavaScriptSourceFrame show inline breakpoints correctly\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');
  await TestRunner.evaluateInPagePromise(`
      function foo()
      {
          var p = Promise.resolve().then(() => console.log(42))
              .then(() => console.log(239));
          return p;
      }

      // some comment.


      // another comment.




      function boo() {
        return 42;
      }
      //# sourceURL=foo.js
    `);

  function waitAndDumpDecorations(sourceFrame) {
    return SourcesTestRunner.waitDebuggerPluginBreakpoints(sourceFrame)
        .then(
            () => SourcesTestRunner.dumpDebuggerPluginBreakpoints(sourceFrame));
  }

  Bindings.breakpointManager._storage._breakpoints = new Map();
  SourcesTestRunner.runDebuggerTestSuite([
    function testAddRemoveBreakpoint(next) {
      var javaScriptSourceFrame;
      SourcesTestRunner.showScriptSource('foo.js', addBreakpoint);

      function addBreakpoint(sourceFrame) {
        javaScriptSourceFrame = sourceFrame;
        TestRunner.addResult('Setting breakpoint');
        SourcesTestRunner.createNewBreakpoint(javaScriptSourceFrame, 11, '', true)
            .then(() => waitAndDumpDecorations(javaScriptSourceFrame).then(removeBreakpoint));
      }

      function removeBreakpoint() {
        TestRunner.addResult('Toggle breakpoint');
        waitAndDumpDecorations(javaScriptSourceFrame).then(() => next());
        SourcesTestRunner.toggleBreakpoint(javaScriptSourceFrame, 11);
      }
    },

    function testAddRemoveBreakpointInLineWithOneLocation(next) {
      var javaScriptSourceFrame;
      SourcesTestRunner.showScriptSource('foo.js', addBreakpoint);

      function addBreakpoint(sourceFrame) {
        javaScriptSourceFrame = sourceFrame;
        TestRunner.addResult('Setting breakpoint');
        SourcesTestRunner.createNewBreakpoint(javaScriptSourceFrame, 13, '', true)
            .then(() => waitAndDumpDecorations(javaScriptSourceFrame).then(removeBreakpoint));
      }

      function removeBreakpoint() {
        TestRunner.addResult('Toggle breakpoint');
        SourcesTestRunner.toggleBreakpoint(javaScriptSourceFrame, 13);
        waitAndDumpDecorations(javaScriptSourceFrame).then(() => next());
      }
    },

    function clickByInlineBreakpoint(next) {
      var javaScriptSourceFrame;
      SourcesTestRunner.showScriptSource('foo.js', addBreakpoint);

      function addBreakpoint(sourceFrame) {
        javaScriptSourceFrame = sourceFrame;
        TestRunner.addResult('Setting breakpoint');
        SourcesTestRunner.createNewBreakpoint(javaScriptSourceFrame, 11, '', true)
            .then(() => waitAndDumpDecorations(javaScriptSourceFrame).then(clickBySecondLocation));
      }

      function clickBySecondLocation() {
        TestRunner.addResult('Click by second breakpoint');
        waitAndDumpDecorations(javaScriptSourceFrame).then(clickByFirstLocation);
        SourcesTestRunner.clickDebuggerPluginBreakpoint(
            javaScriptSourceFrame, 11, 1, next);
      }

      function clickByFirstLocation() {
        TestRunner.addResult('Click by first breakpoint');
        waitAndDumpDecorations(javaScriptSourceFrame).then(clickBySecondLocationAgain);
        SourcesTestRunner.clickDebuggerPluginBreakpoint(
            javaScriptSourceFrame, 11, 0, next);
      }

      function clickBySecondLocationAgain() {
        TestRunner.addResult('Click by second breakpoint');
        waitAndDumpDecorations(javaScriptSourceFrame).then(() => next());
        SourcesTestRunner.clickDebuggerPluginBreakpoint(
            javaScriptSourceFrame, 11, 1, next);
      }
    },

    function toggleBreakpointInAnotherLineWontRemoveExisting(next) {
      var javaScriptSourceFrame;
      SourcesTestRunner.showScriptSource('foo.js', addBreakpoint);

      function addBreakpoint(sourceFrame) {
        javaScriptSourceFrame = sourceFrame;
        TestRunner.addResult('Setting breakpoint in line 4');
        SourcesTestRunner.toggleBreakpoint(sourceFrame, 12, false);
        waitAndDumpDecorations(javaScriptSourceFrame).then(toggleBreakpointInAnotherLine);
      }

      function toggleBreakpointInAnotherLine() {
        TestRunner.addResult('Setting breakpoint in line 3');
        waitAndDumpDecorations(javaScriptSourceFrame).then(removeBreakpoints);
        SourcesTestRunner.toggleBreakpoint(javaScriptSourceFrame, 11, false);
      }

      function removeBreakpoints() {
        TestRunner.addResult('Click by first inline breakpoints');
        waitAndDumpDecorations(javaScriptSourceFrame).then(() => next());
        SourcesTestRunner.clickDebuggerPluginBreakpoint(
            javaScriptSourceFrame, 11, 0, next);
        SourcesTestRunner.clickDebuggerPluginBreakpoint(
            javaScriptSourceFrame, 12, 0, next);
      }
    },

    async function testAddRemoveBreakpointInLineWithoutBreakableLocations(next) {
      let javaScriptSourceFrame = await SourcesTestRunner.showScriptSourcePromise('foo.js');

      TestRunner.addResult('Setting breakpoint');
      await SourcesTestRunner.createNewBreakpoint(javaScriptSourceFrame, 16, '', true)
      await waitAndDumpDecorations(javaScriptSourceFrame);

      TestRunner.addResult('Toggle breakpoint');
      let decorationsPromise = waitAndDumpDecorations(javaScriptSourceFrame);
      SourcesTestRunner.toggleBreakpoint(javaScriptSourceFrame, 25);
      await decorationsPromise;
      next();
    }
  ]);
})();
