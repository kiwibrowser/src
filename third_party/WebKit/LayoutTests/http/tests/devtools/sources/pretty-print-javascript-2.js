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
    function forLoopWithIfStatementWithoutBlockStatements(next) {
      var mappingQueries = ['length', 'console', 'of'];
      testJSFormatter('for(var value of map)if (value.length%3===0)console.log(value);', mappingQueries, next);
    },

    function objectExpressionProperties(next) {
      var mappingQueries = ['mapping', 'original', 'formatted'];
      testJSFormatter('var mapping={original:[1,2,3],formatted:[],count:0}', mappingQueries, next);
    },

    function blockFormatting(next) {
      var mappingQueries = ['(1)', '(2)'];
      testJSFormatter('{ print(1); print(2); }', mappingQueries, next);
    },

    function assignmentFormatting(next) {
      var mappingQueries = ['string'];
      testJSFormatter('var exp=\'a string\';c=+a+(0>a?b:0);c=(1);var a=(1);', mappingQueries, next);
    },

    function objectLiteralFormatting(next) {
      var mappingQueries = ['dog', '1989', 'foo'];
      testJSFormatter('var obj={\'foo\':1,bar:"2",cat:{dog:\'1989\'}}', mappingQueries, next);
    },
  ]);
})();
