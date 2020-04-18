// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that scripts panel UI elements work as intended.\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.loadModule('sdk_test_runner');
  await TestRunner.showPanel('sources');

  function dumpNavigator(sourcesNavigatorView) {
    TestRunner.addResult('Navigator:');
    SourcesTestRunner.dumpNavigatorView(sourcesNavigatorView);
  }

  function createNavigatorView() {
    var navigatorView = new Sources.NetworkNavigatorView();
    navigatorView.show(UI.inspectorView.element);
    return navigatorView;
  }

  TestRunner.addSniffer(Workspace.UISourceCode.prototype, 'requestContent', onRequestContent, true);

  function onRequestContent() {
    TestRunner.addResult('Source requested for ' + this.url());
  }

  Bindings.debuggerWorkspaceBinding._resetForTest(TestRunner.mainTarget);
  Bindings.resourceMapping._resetForTest(TestRunner.mainTarget);
  var page = new SDKTestRunner.PageMock('http://example.com');
  SDKTestRunner.connectToPage('mock-page', page, true /* makeMainTarget */);

  function addNetworkFile(url) {
    page.evalScript(url, '', false /* isContentScript */);
    return TestRunner.waitForUISourceCode(url);
  }

  function reload() {
    page.reload();
    return new Promise(fulfill => TestRunner.addSniffer(SDK.ResourceTreeModel.prototype, '_frameNavigated', fulfill));
  }

  TestRunner.runTestSuite([
    async function testInitialLoad(next) {
      await reload();
      await addNetworkFile('foobar.js');

      var sourcesNavigatorView = createNavigatorView();

      await addNetworkFile('foo.js');
      await addNetworkFile('bar.js');
      var uiSourceCode = await addNetworkFile('baz.js');
      sourcesNavigatorView.revealUISourceCode(uiSourceCode);

      dumpNavigator(sourcesNavigatorView);
      next();
    },

    async function testReset(next) {
      await reload();
      var sourcesNavigatorView = createNavigatorView();

      var uiSourceCode = await addNetworkFile('foo.js');
      await addNetworkFile('bar.js');
      await addNetworkFile('baz.js');

      dumpNavigator(sourcesNavigatorView);
      TestRunner.addResult('Revealing in navigator.');
      sourcesNavigatorView.revealUISourceCode(uiSourceCode);
      dumpNavigator(sourcesNavigatorView);

      await reload();
      dumpNavigator(sourcesNavigatorView);
      uiSourceCode = await addNetworkFile('bar.js');
      sourcesNavigatorView.revealUISourceCode(uiSourceCode);
      dumpNavigator(sourcesNavigatorView);

      next();
    },

    async function testDebuggerUISourceCodeAddedAndRemoved(next) {
      await reload();
      var sourcesNavigatorView = createNavigatorView();

      var uiSourceCode = await addNetworkFile('foo.js');
      TestRunner.waitForUISourceCode().then(onUISourceCode);
      TestRunner.evaluateInPageAnonymously('function foo() {}');

      async function onUISourceCode(debuggerUISourceCode) {
        sourcesNavigatorView.revealUISourceCode(uiSourceCode);
        sourcesNavigatorView.revealUISourceCode(debuggerUISourceCode);
        dumpNavigator(sourcesNavigatorView);

        // Plug compiler source mapping.
        await addNetworkFile('source.js');

        dumpNavigator(sourcesNavigatorView);
        next();
      }
    }
  ]);
})();
