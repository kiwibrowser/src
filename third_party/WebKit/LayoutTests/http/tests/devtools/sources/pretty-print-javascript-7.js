// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Verifies JavaScript pretty-printing functionality.\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');

  var testJSFormatter = SourcesTestRunner.testPrettyPrint.bind(SourcesTestRunner, 'text/javascript');

  TestRunner.runTestSuite([
    function semicolonAfterFunctionExpression(next) {
      var mappingQueries = ['onClick', 'function', 'console', 'log', 'click!', 'done'];
      testJSFormatter(
          'var onClick = function() { console.log(\'click!\'); };console.log(\'done\');', mappingQueries, next);
    },

    function semicolonAfterMultipleFunctionExpressions(next) {
      var mappingQueries = ['onStart', 'onFinish', 'a()', 'b()'];
      testJSFormatter('var onStart = function() { a(); }, onFinish = function() { b(); };', mappingQueries, next);
    },

    function semicolonAfterEmptyFunctionExpressions(next) {
      var mappingQueries = ['onStart', 'delay', '1000', 'belay', 'activeElement'];
      testJSFormatter('var onStart = function() {}, delay=1000, belay=document.activeElement;', mappingQueries, next);
    },

    function continueStatementFormatting(next) {
      var mappingQueries = ['function', '1', 'continue', 'test'];
      testJSFormatter('function foo(){while(1){if (a)continue;test();}}', mappingQueries, next);
    },

    function inconsistentSpaceAfterNull(next) {
      var mappingQueries = ['||', 'null', ';'];
      testJSFormatter('1||null;', mappingQueries, next);
    },

    function squashMultipleNewlines(next) {
      var mappingQueries = ['a', 'b'];
      testJSFormatter('a();\n\n\n\n\n\n\n\n\nb();', mappingQueries, next);
    },

    function ensureExponentialOperator(next) {
      var mappingQueries = ['2', '**', '3'];
      testJSFormatter('2**3', mappingQueries, next);
    },
  ]);
})();
