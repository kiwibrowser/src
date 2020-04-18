// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  await TestRunner.setupStartupTest('resources/database-open.html');
  TestRunner.addResult(
      `Tests that Web Inspector gets populated with databases that were opened before inspector is shown.\n`);
  await TestRunner.loadModule('application_test_runner');

  ApplicationTestRunner.databaseModel().enable();
  function run() {
    function compareByName(d1, d2) {
      if (d1.name < d2.name)
        return -1;
      if (d1.name > d2.name)
        return 1;
      return 0;
    }

    var databases = ApplicationTestRunner.databaseModel().databases();
    databases.sort(compareByName);
    for (var i = 0; i < databases.length; ++i) {
      TestRunner.addResult('Name: ' + databases[i].name);
      TestRunner.addResult('Version: ' + databases[i].version);
      TestRunner.addResult('Domain: ' + databases[i].domain);
    }
    TestRunner.completeTest();
  }
  TestRunner.deprecatedRunAfterPendingDispatches(run);
})();
