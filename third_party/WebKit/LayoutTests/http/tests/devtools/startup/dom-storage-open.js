// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.setupStartupTest('resources/dom-storage-open.html');
  TestRunner.addResult(
      `Tests that Web Inspector gets populated with DOM storages according to security origins found in the page.\n`);
  await TestRunner.loadModule('application_test_runner');
  function callback() {
    ApplicationTestRunner.domStorageModel().enable();
    var storages = ApplicationTestRunner.domStorageModel().storages();
    for (var i = 0; i < storages.length; ++i) {
      TestRunner.addResult('SecurityOrigin: ' + storages[i].securityOrigin);
      TestRunner.addResult('IsLocalStorage: ' + storages[i].isLocalStorage);
    }
    TestRunner.completeTest();
  }
  TestRunner.deprecatedRunAfterPendingDispatches(callback);
})();
