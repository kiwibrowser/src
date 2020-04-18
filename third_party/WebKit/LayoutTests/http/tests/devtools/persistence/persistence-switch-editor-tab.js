// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(
      `Verify that a network file tab gets substituted with filesystem tab when persistence binding comes.\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.loadModule('bindings_test_runner');
  await TestRunner.showPanel('sources');
  await TestRunner.addScriptTag('resources/foo.js');

  var testMapping = BindingsTestRunner.initializeTestMapping();
  TestRunner.runTestSuite([
    function openNetworkTab(next) {
      TestRunner.waitForUISourceCode('foo.js', Workspace.projectTypes.Network)
          .then(sourceCode => SourcesTestRunner.showUISourceCodePromise(sourceCode))
          .then(onSourceFrame);

      function onSourceFrame(sourceFrame) {
        dumpEditorTabs();
        next();
      }
    },

    function addMapping(next) {
      var fs = new BindingsTestRunner.TestFileSystem('file:///var/www');
      BindingsTestRunner.addFooJSFile(fs);
      fs.reportCreated(function() {});
      testMapping.addBinding('foo.js');
      BindingsTestRunner.waitForBinding('foo.js').then(onBindingCreated);

      function onBindingCreated() {
        dumpEditorTabs();
        next();
      }
    },
  ]);

  function dumpEditorTabs() {
    var editorContainer = UI.panels.sources._sourcesView._editorContainer;
    var openedUISourceCodes = editorContainer._tabIds.keysArray();
    openedUISourceCodes.sort((a, b) => a.url().compareTo(b.url()));
    TestRunner.addResult('Opened tabs: ');
    for (code of openedUISourceCodes)
      TestRunner.addResult('    ' + code.url());
  }
})();
