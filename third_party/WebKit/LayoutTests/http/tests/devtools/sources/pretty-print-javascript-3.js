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
    function ifStatements(next) {
      var mappingQueries = ['===', '!==', 'non-eq'];
      testJSFormatter(
          'if(a<b)log(a);else log(b);if(a<b){log(a)}else{log(b);}if(a===b)log(\'equals\');if(a!==b){log(\'non-eq\');}',
          mappingQueries, next);
    },

    function arrayLiteralFormatting(next) {
      var mappingQueries = ['3', '2', '1', '0'];
      testJSFormatter('var arr=[3,2,1,0]', mappingQueries, next);
    },

    function ifFormatting(next) {
      var mappingQueries = ['&&', 'print(a)'];
      testJSFormatter('if(a>b&&b>c){print(a);print(b);}', mappingQueries, next);
    },

    function ternarOperatorFormatting(next) {
      var mappingQueries = ['?', ':'];
      testJSFormatter('a>b?a:b', mappingQueries, next);
    },

    function labeledStatementFormatting(next) {
      var mappingQueries = ['break', 'continue', 'while'];
      testJSFormatter('firstLoop:while(true){break firstLoop;continue firstLoop;}', mappingQueries, next);
    },
  ]);
})();
