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
    function trailingCommentsTest(next) {
      var mappingQueries = [];
      testJSFormatter('noop(); //#sourceMappingURL=program.js.map', mappingQueries, next);
    },

    function inlinedScriptFormatting(next) {
      var content = '<html><body><script>function f(){}<' +
          '/script><script>function g(){var a;window.return = 10;if (a) return;}<' +
          '/script></body></html>';
      SourcesTestRunner.testPrettyPrint('text/html', content, [], next);
    },

    function generatorFormatter(next) {
      var mappingQueries = ['max', '*', 'else'];
      testJSFormatter(
          'function *max(){var a=yield;var b=yield 10;if(a>b)return a;else return b;}', mappingQueries, next);
    },

    function blockCommentFormatter(next) {
      var mappingQueries = ['this', 'is', 'block', 'comment', 'var', '10'];
      testJSFormatter('/** this\n * is\n * block\n * comment\n */\nvar a=10;', mappingQueries, next);
    },

    function lexicalScoping(next) {
      var mappingQueries = ['for', 'person', 'name', '}'];
      testJSFormatter(
          'for(var i=0;i<names.length;++i){let name=names[i];let person=persons[i];}', mappingQueries, next);
    },

    function anonimousFunctionAsParameter(next) {
      var mappingQueries = ['setTimeout', 'function', 'alert', '2000'];
      testJSFormatter('setTimeout(function(){alert(1);},2000);', mappingQueries, next);
    },

    function arrowFunction(next) {
      var mappingQueries = ['function', 'console', '=>', '2'];
      testJSFormatter('function test(arg){console.log(arg);}test(a=>a+2);', mappingQueries, next);
    }
  ]);
})();
