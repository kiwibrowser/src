// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Verify that breakpoints are moved appropriately in case of page reload.\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.loadModule('bindings_test_runner');
  await TestRunner.showPanel('sources');
  await TestRunner.evaluateInPagePromise(`
      function addFooJS() {
          var script = document.createElement('script');
          script.src = '${TestRunner.url("./resources/foo.js")}';
          document.body.appendChild(script);
      }
  `);

  var testMapping = BindingsTestRunner.initializeTestMapping();
  var fs = new BindingsTestRunner.TestFileSystem('file:///var/www');
  BindingsTestRunner.addFooJSFile(fs);

  TestRunner.runTestSuite([
    function addFileSystem(next) {
      fs.reportCreated(next);
    },

    function addNetworkFooJS(next) {
      TestRunner.evaluateInPage('addFooJS()');
      testMapping.addBinding('foo.js');
      BindingsTestRunner.waitForBinding('foo.js').then(next);
    },

    function setBreakpointInFileSystemUISourceCode(next) {
      TestRunner.waitForUISourceCode('foo.js', Workspace.projectTypes.FileSystem)
          .then(sourceCode => SourcesTestRunner.showUISourceCodePromise(sourceCode))
          .then(onSourceFrame);

      function onSourceFrame(sourceFrame) {
        SourcesTestRunner.setBreakpoint(sourceFrame, 0, '', true);
        SourcesTestRunner.waitBreakpointSidebarPane(true).then(dumpBreakpointSidebarPane).then(next);
      }
    },

    async function reloadPageAndDumpBreakpoints(next) {
      testMapping.removeBinding('foo.js');
      await Promise.all([SourcesTestRunner.waitBreakpointSidebarPane(), TestRunner.reloadPagePromise()]);
      testMapping.addBinding('foo.js');
      dumpBreakpointSidebarPane();
      next();
    },
  ]);

  function dumpBreakpointSidebarPane() {
    var paneElement = self.runtime.sharedInstance(Sources.JavaScriptBreakpointsSidebarPane).contentElement;
    var empty = paneElement.querySelector('.gray-info-message');
    if (empty)
      return TestRunner.textContentWithLineBreaks(empty);
    var entries = Array.from(paneElement.querySelectorAll('.breakpoint-entry'));
    for (var entry of entries) {
      var uiLocation = entry[Sources.JavaScriptBreakpointsSidebarPane._locationSymbol];
      TestRunner.addResult('    ' + uiLocation.uiSourceCode.url() + ':' + uiLocation.lineNumber);
    }
  }
})();
