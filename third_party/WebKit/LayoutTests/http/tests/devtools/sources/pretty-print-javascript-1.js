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
    function multipleVariableDeclarations(next) {
      var mappingQueries = ['{}', 'hello', ';'];
      testJSFormatter('var a=1,b={},c=2,d="hello world";var a,b,c,d=2,e,f=3;', mappingQueries, next);
    },

    function emptyObjectExpression(next) {
      var mappingQueries = ['{', '}', 'a'];
      testJSFormatter('var a={}', mappingQueries, next);
    },

    function breakContinueStatements(next) {
      var mappingQueries = ['break', 'continue', '2', 'else'];
      testJSFormatter('for(var i in set)if(i%2===0)break;else continue;', mappingQueries, next);
    },

    function chainedIfStatements(next) {
      var mappingQueries = ['7', '9', '3', '++'];
      testJSFormatter(
          'if(a%7===0)b=1;else if(a%9===1) b =  2;else if(a%5===3){b=a/2;b++;} else b= 3;', mappingQueries, next);
    },

    function tryCatchStatement(next) {
      var mappingQueries = ['try', 'catch', 'finally'];
      testJSFormatter('try{a(b());}catch(e){f()}finally{f();}', mappingQueries, next);
    },
  ]);
})();
