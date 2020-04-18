// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult('Tests breakpoints on navigation.');
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');

  await TestRunner.navigate(TestRunner.url('resources/a.html'));

  let sourceFrame = await SourcesTestRunner.showScriptSourcePromise('a.html');
  TestRunner.addResult('Set different breakpoints in inline script and dump them');
  SourcesTestRunner.toggleBreakpoint(sourceFrame, 3, false);
  SourcesTestRunner.createNewBreakpoint(sourceFrame, 5, 'a === 3', true);
  SourcesTestRunner.createNewBreakpoint(sourceFrame, 6, '', false);
  await SourcesTestRunner.waitDebuggerPluginBreakpoints(sourceFrame);
  SourcesTestRunner.dumpDebuggerPluginBreakpoints(sourceFrame);

  sourceFrame = await SourcesTestRunner.showScriptSourcePromise('a.js');
  TestRunner.addResult('Set different breakpoints and dump them');
  SourcesTestRunner.toggleBreakpoint(sourceFrame, 9, false);
  SourcesTestRunner.createNewBreakpoint(sourceFrame, 10, 'a === 3', true);
  SourcesTestRunner.createNewBreakpoint(sourceFrame, 5, '', false);
  await SourcesTestRunner.waitDebuggerPluginBreakpoints(sourceFrame);
  SourcesTestRunner.dumpDebuggerPluginBreakpoints(sourceFrame);

  TestRunner.addResult('Dump to b.html and check that there is no breakpoints');
  await TestRunner.navigate(TestRunner.url('resources/b.html'));
  sourceFrame = await SourcesTestRunner.showScriptSourcePromise('b.html');
  SourcesTestRunner.dumpDebuggerPluginBreakpoints(sourceFrame);

  TestRunner.addResult('Navigate back to a.html and dump breakpoints');
  await TestRunner.navigate(TestRunner.url('resources/a.html'));
  TestRunner.addResult('a.html:');
  sourceFrame = await SourcesTestRunner.showScriptSourcePromise('a.html');
  await SourcesTestRunner.waitDebuggerPluginBreakpoints(sourceFrame);
  SourcesTestRunner.dumpDebuggerPluginBreakpoints(sourceFrame);
  TestRunner.addResult('a.js:');
  sourceFrame = await SourcesTestRunner.showScriptSourcePromise('a.js');
  await SourcesTestRunner.waitDebuggerPluginBreakpoints(sourceFrame);
  SourcesTestRunner.dumpDebuggerPluginBreakpoints(sourceFrame);

  TestRunner.completeTest();
})();
