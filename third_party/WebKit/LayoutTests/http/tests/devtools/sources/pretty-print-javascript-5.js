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
    function forInFormatting(next) {
      var mappingQueries = ['myMap', 'print'];
      testJSFormatter('for(var key in myMap)print(key);', mappingQueries, next);
    },

    function forOfFormatting(next) {
      var mappingQueries = ['myMap', 'print'];
      testJSFormatter('for(var value of myMap)print(value);', mappingQueries, next);
    },

    function commaBetweenStatementsFormatting(next) {
      var mappingQueries = ['noop', 'hasNew'];
      testJSFormatter('rebuild(),show(),hasNew?refresh():noop();', mappingQueries, next);
    },

    function complexScriptFormatting(next) {
      SourcesTestRunner.showScriptSource('obfuscated.js', didShowScriptSource);

      function didShowScriptSource(sourceFrame) {
        var mappingQueries = [
          'function', 'formatted1', 'variable1', '    return "functionWithComments"', 'onmessage', 'indent_start',
          'function require', 'var regexp', 'importScripts', 'formatted2'
        ];
        testJSFormatter(sourceFrame._textEditor.text(), mappingQueries, next);
      }
    },

    function ifStatementIndentRegression(next) {
      var mappingQueries = ['pretty', 'reset'];
      testJSFormatter('{if (a>b){a();pretty();}else if (a+b)e();reset();}', mappingQueries, next);
    },
  ]);
})();
