// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Editing inline styles should play nice with inline scripts.\n`);
  await TestRunner.loadModule('bindings_test_runner');

  await TestRunner.navigatePromise('./resources/inline-style.html');
  var uiSourceCode = await TestRunner.waitForUISourceCode('inline-style.html', Workspace.projectTypes.Network);

  await uiSourceCode.requestContent(); // prefetch content to fix flakiness
  var styleSheets = TestRunner.cssModel.styleSheetIdsForURL(uiSourceCode.url());
  var scripts = TestRunner.debuggerModel.scriptsForSourceURL(uiSourceCode.url());
  var locationPool = new Bindings.LiveLocationPool();
  var i = 0;
  for (var script of scripts) {
    var rawLocation = TestRunner.debuggerModel.createRawLocation(script, script.lineOffset, script.columnOffset);
    Bindings.debuggerWorkspaceBinding.createLiveLocation(
      rawLocation, updateDelegate.bind(null, 'script' + i), locationPool);
    i++;
  }

  i = 0;
  for (var styleSheetId of styleSheets) {
    var header = TestRunner.cssModel.styleSheetHeaderForId(styleSheetId);
    var rawLocation = new SDK.CSSLocation(header, header.startLine, header.startColumn);
    Bindings.cssWorkspaceBinding.createLiveLocation(
      rawLocation, updateDelegate.bind(null, 'style' + i), locationPool);
    i++;
  }

  i = 0;
  for (var styleSheetId of styleSheets) {
    TestRunner.addResult('Adding rule' + i)
    await TestRunner.cssModel.addRule(styleSheetId, `.new-rule {
  --new: true;
}`, TextUtils.TextRange.createFromLocation(0, 0));
    await Promise.resolve();
    i++;
  }


  function updateDelegate(name, location) {
    var uiLocation = location.uiLocation();
    TestRunner.addResult(`LiveLocation '${name}' was updated ${uiLocation.lineNumber}:${uiLocation.columnNumber}`);
  }

  TestRunner.completeTest();
})();
