// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult('Tests disabled breakpoint.');
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');

  await TestRunner.addScriptTag('resources/a.js');

  await SourcesTestRunner.startDebuggerTestPromise();
  const sourceFrame = await SourcesTestRunner.showScriptSourcePromise('a.js');
  TestRunner.addResult('Set breakpoint');
  SourcesTestRunner.toggleBreakpoint(sourceFrame, 9, false);
  await SourcesTestRunner.waitDebuggerPluginBreakpoints(sourceFrame);

  TestRunner.addResult('Run function and check pause');
  TestRunner.evaluateInPage('main()//# sourceURL=test.js');
  SourcesTestRunner.captureStackTrace(await SourcesTestRunner.waitUntilPausedPromise());
  await new Promise(resolve => SourcesTestRunner.resumeExecution(resolve));

  TestRunner.addResult('Disable breakpoint');
  SourcesTestRunner.toggleBreakpoint(sourceFrame, 9, true);

  TestRunner.addResult('Run function and check that pause happens after function');
  TestRunner.evaluateInPage('main(); debugger;//# sourceURL=test.js');
  SourcesTestRunner.captureStackTrace(await SourcesTestRunner.waitUntilPausedPromise());
  await new Promise(resolve => SourcesTestRunner.resumeExecution(resolve));

  SourcesTestRunner.completeDebuggerTest();
})();
