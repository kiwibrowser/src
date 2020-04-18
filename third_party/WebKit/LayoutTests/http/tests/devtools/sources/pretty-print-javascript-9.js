// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Verifies JavaScript pretty-printing functionality.\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');

  var testJSFormatter = SourcesTestRunner.testPrettyPrint.bind(SourcesTestRunner, 'text/javascript');

  TestRunner.runTestSuite([
    function parenthesizedExpressions(next) {
      var mappingQueries = ['if', '((a))', '((b));', 'else', '(c)'];
      testJSFormatter('if((a))((b));else (c);', mappingQueries, next);
    },

    function objectDesctructuring(next) {
      var mappingQueries = ['let', 'y', 'getXYFromTouchOrPointer', 'e'];
      testJSFormatter('let{x,y}=getXYFromTouchOrPointer(e);', mappingQueries, next);
    },

    function objectDesctructuringInFunctionExpression(next) {
      var mappingQueries = ['test', 'function', 'foo'];
      testJSFormatter('var test = function({x,y}){foo(x,y);}', mappingQueries, next);
    },
  ]);
})();
