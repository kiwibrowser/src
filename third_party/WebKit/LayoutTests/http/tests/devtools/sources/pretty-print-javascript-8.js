// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Verifies JavaScript pretty-printing functionality.\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');

  var testJSFormatter = SourcesTestRunner.testPrettyPrint.bind(SourcesTestRunner, 'text/javascript');

  TestRunner.runTestSuite([
    function asyncAwaitSupport(next) {
      var mappingQueries = ['async', 'function', 'foo', 'return', 'Promise', 'resolve'];
      testJSFormatter('async function foo() {return await Promise.resolve(1);}', mappingQueries, next);
    },
  ]);
})();
