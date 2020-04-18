// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`This test validates set of loaded modules for Sources panel.\n`);


  var initialModules = TestRunner.loadedModules();
  await UI.inspectorView.panel('sources');
  var sourcesModules = TestRunner.dumpLoadedModules(initialModules);

  await TestRunner.addScriptTag('resources/bar.js');
  var resource;
  TestRunner.resourceTreeModel.forAllResources(function(r) {
    if (r.url.indexOf('bar.js') !== -1) {
      resource = r;
      return true;
    }
  });
  TestRunner.addResult('Now with source code opened');
  var uiLocation = Workspace.workspace.uiSourceCodeForURL(resource.url).uiLocation(2, 1);
  Common.Revealer.reveal(uiLocation);
  TestRunner.dumpLoadedModules(sourcesModules);
  TestRunner.completeTest();
})();
