// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`This test validates set of loaded modules for Network panel.\n`);


  var initialModules = TestRunner.loadedModules();
  await UI.inspectorView.panel('network');
  TestRunner.dumpLoadedModules(initialModules);
  TestRunner.completeTest();
})();
