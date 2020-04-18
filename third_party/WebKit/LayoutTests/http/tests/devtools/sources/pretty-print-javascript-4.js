// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Verifies JavaScript pretty-printing functionality.\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');
  await TestRunner.addScriptTag('debugger/resources/obfuscated.js');

  var testJSFormatter = SourcesTestRunner.testPrettyPrint.bind(SourcesTestRunner, 'text/javascript');

  TestRunner.runTestSuite([
    function withStatementFormatting(next) {
      var mappingQueries = ['first', 'obj', 'nice', '1', '2', 'done'];
      testJSFormatter('with(obj)log(\'first\');with(nice){log(1);log(2);}done();', mappingQueries, next);
    },

    function switchStatementFormatting(next) {
      var mappingQueries = ['even', 'odd', '89', 'done'];
      testJSFormatter(
          'switch (a) { case 1, 3: log("odd");break;case 2:log("even");break;case 42:case 89: log(a);default:log("interesting");log(a);}log("done");',
          mappingQueries, next);
    },

    function whileFormatting(next) {
      var mappingQueries = ['while', 'infinity', ');'];
      testJSFormatter('while(true){print(\'infinity\');}', mappingQueries, next);
    },

    function doWhileFormatting(next) {
      var mappingQueries = ['while', 'infinity'];
      testJSFormatter('do{print(\'infinity\');}while(true);', mappingQueries, next);
    },

    function functionFormatting(next) {
      var mappingQueries = ['return', '*='];
      testJSFormatter('function test(a,b,c){a*=b;return c+a;}', mappingQueries, next);
    },
  ]);
})();
