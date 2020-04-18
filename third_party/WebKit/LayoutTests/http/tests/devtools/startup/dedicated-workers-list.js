// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  await TestRunner.setupStartupTest('resources/dedicated-workers-list.html');
  TestRunner.addResult(
      `Tests that dedicated workers created before worker inspection was enabled will be reported to the front-end. Bug 72020 https://bugs.webkit.org/show_bug.cgi?id=72020\n`);
  await TestRunner.loadModule('sources_test_runner');
  var workerCount = 0;
  var observer = {
    targetAdded(target) {
      if (!TestRunner.isDedicatedWorker(target))
        return;
      workerCount++;
      TestRunner.addResult('Added worker: ' + workerCount);
      if (workerCount === 2) {
        TestRunner.addResult('Done.');
        TestRunner.completeTest();
      }
    },

    targetRemoved() {}
  };

  await TestRunner.TargetAgent.setAutoAttach(false, false);
  // Debugger should not crash when autoconnecting is immediately followed by termination.

  SDK.targetManager.observeTargets(observer);
  TestRunner.TargetAgent.setAutoAttach(true, true);
  TestRunner.addResult('Worker inspection enabled');
})();
