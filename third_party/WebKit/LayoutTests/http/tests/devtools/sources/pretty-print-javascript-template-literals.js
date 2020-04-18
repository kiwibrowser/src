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
    function simpleLiteral(next) {
      var mappingQueries = ['foo', 'bar'];
      testJSFormatter('var foo = `bar`;', mappingQueries, next);
    },

    function multilineLiteral(next) {
      var mappingQueries = ['foo', 'bar'];
      testJSFormatter('var foo = `this\nbar`;', mappingQueries, next);
    },

    function stringSubstitution(next) {
      var mappingQueries = ['credit', 'cash'];
      testJSFormatter('var a=`I have ${credit+cash}$`;', mappingQueries, next);
    },

    function multipleStringSubstitution(next) {
      var mappingQueries = ['credit', 'cash'];
      testJSFormatter('var a=`${name} has ${credit+cash}${currency?currency:"$"}`;', mappingQueries, next);
    },

    function taggedTemplate(next) {
      var mappingQueries = ['escapeHtml', 'width'];
      testJSFormatter('escapeHtml`<div class=${classnName} width=${a+b}/>`;', mappingQueries, next);
    },

    function escapedApostrophe(next) {
      var mappingQueries = ['That', 'great'];
      testJSFormatter('var a=`That\`s great!`;', mappingQueries, next);
    }
  ]);
})();
