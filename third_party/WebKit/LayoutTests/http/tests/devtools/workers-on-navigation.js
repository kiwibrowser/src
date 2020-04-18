// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that workers are correctly detached upon navigation.\n`);

  // Suppress the following protocol error from being printed (which had a race condition):
  // error: Connection is closed, can't dispatch pending Debugger.setBlackboxPatterns
  console.error = () => undefined;

  var workerTargetId;
  var navigated = false;
  var observer = {
    targetAdded(target) {
      if (!TestRunner.isDedicatedWorker(target))
        return;
      TestRunner.addResult('Worker added');
      workerTargetId = target.id();
      if (navigated)
        TestRunner.completeTest();
    },
    targetRemoved(target) {
      if (!TestRunner.isDedicatedWorker(target))
        return;
      if (target.id() === workerTargetId) {
        TestRunner.addResult('Worker removed');
        workerTargetId = '';
      } else {
        TestRunner.addResult('Unknown worker removed');
      }
    }
  };
  SDK.targetManager.observeTargets(observer);
  await TestRunner.navigatePromise('resources/workers-on-navigation-resource.html');
  await TestRunner.evaluateInPagePromise('startWorker()');
  await TestRunner.reloadPagePromise();
  navigated = true;
  await TestRunner.evaluateInPagePromise('startWorker()');
})();
