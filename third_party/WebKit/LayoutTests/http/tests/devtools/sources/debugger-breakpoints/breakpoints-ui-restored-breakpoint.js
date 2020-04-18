// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult('Tests breakpoint is restored.');
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');

  await TestRunner.addScriptTag('resources/a.js');

  let originalSourceFrame = await SourcesTestRunner.showScriptSourcePromise('a.js');
  TestRunner.addResult('Set different breakpoints and dump them');
  SourcesTestRunner.toggleBreakpoint(originalSourceFrame, 9, false);
  SourcesTestRunner.createNewBreakpoint(originalSourceFrame, 10, 'a === 3', true);
  SourcesTestRunner.createNewBreakpoint(originalSourceFrame, 5, '', false);
  await SourcesTestRunner.waitDebuggerPluginBreakpoints(originalSourceFrame);
  await SourcesTestRunner.waitUntilDebuggerPluginLoaded(originalSourceFrame);
  SourcesTestRunner.dumpDebuggerPluginBreakpoints(originalSourceFrame);

  TestRunner.addResult('Reload page and add script again and dump breakpoints');
  await TestRunner.reloadPagePromise();
  await TestRunner.addScriptTag(TestRunner.url('resources/a.js'));
  let sourceFrameAfterReload = await SourcesTestRunner.showScriptSourcePromise('a.js');
  await SourcesTestRunner.waitDebuggerPluginBreakpoints(sourceFrameAfterReload);
  await SourcesTestRunner.waitUntilDebuggerPluginLoaded(sourceFrameAfterReload);
  SourcesTestRunner.dumpDebuggerPluginBreakpoints(sourceFrameAfterReload);

  // TODO(kozyatinskiy): as soon as we have script with the same url in different frames
  // everything looks compeltely broken, we should fix it.
  TestRunner.addResult('Added two more iframes with script with the same url');
  TestRunner.addIframe(TestRunner.url('resources/frame-with-script.html'));
  TestRunner.addIframe(TestRunner.url('resources/frame-with-script.html'));
  const uiSourceCodes = await waitForNScriptSources('a.js', 3);
  for (const uiSourceCode of uiSourceCodes) {
    TestRunner.addResult('Show uiSourceCode and dump breakpoints');
    const sourceFrame = await SourcesTestRunner.showUISourceCodePromise(uiSourceCode);
    await SourcesTestRunner.waitUntilDebuggerPluginLoaded(sourceFrame);
    SourcesTestRunner.dumpDebuggerPluginBreakpoints(sourceFrame);
  }

  TestRunner.addResult('Reload page and add script again and dump breakpoints');
  await TestRunner.reloadPagePromise();
  await TestRunner.addScriptTag(TestRunner.url('resources/a.js'));
  sourceFrameAfterReload = await SourcesTestRunner.showScriptSourcePromise('a.js');
  await SourcesTestRunner.waitUntilDebuggerPluginLoaded(sourceFrameAfterReload);
  SourcesTestRunner.dumpDebuggerPluginBreakpoints(sourceFrameAfterReload);

  TestRunner.completeTest();

  async function waitForNScriptSources(scriptName, N) {
    while (true) {
      let uiSourceCodes = UI.panels.sources._workspace.uiSourceCodes();
      uiSourceCodes = uiSourceCodes.filter(uiSourceCode => uiSourceCode.project().type() !== Workspace.projectTypes.Service && uiSourceCode.name() === scriptName);
      if (uiSourceCodes.length === N)
        return uiSourceCodes;
      await TestRunner.addSnifferPromise(Sources.SourcesView.prototype, '_addUISourceCode');
    }
  }
})();
