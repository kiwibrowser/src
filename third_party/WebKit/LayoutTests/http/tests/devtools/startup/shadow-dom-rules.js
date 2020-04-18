// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  await TestRunner.setupStartupTest('resources/shadow-dom-rules.html');
  TestRunner.addResult(`This test checks that style sheets hosted inside shadow roots could be inspected.\n`);
  await TestRunner.loadModule('elements_test_runner');

  TestRunner.runTestSuite([
    function testInit(next) {
      ElementsTestRunner.selectNodeAndWaitForStyles('inner', next);
    },

    function testDumpStyles(next) {
      ElementsTestRunner.dumpSelectedElementStyles(true);
      next();
    }
  ]);
})();
