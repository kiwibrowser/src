// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`This test validates initial set of loaded modules for Elements panel.\n`);


  var initialModules = TestRunner.loadedModules();
  await UI.inspectorView.panel('elements');
  var elementsModules = TestRunner.dumpLoadedModules(initialModules);
  TestRunner.addResult('Now with animations pane');
  await self.runtime.loadModulePromise('animation');
  TestRunner.dumpLoadedModules(elementsModules);
  TestRunner.completeTest();
})();
