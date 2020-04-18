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
    function emptyClass(next) {
      var mappingQueries = ['Test', 'class'];
      testJSFormatter('class Test{}', mappingQueries, next);
    },

    function emptyConstructor(next) {
      var mappingQueries = ['Test', 'class', 'constructor'];
      testJSFormatter('class Test{constructor(){}}', mappingQueries, next);
    },

    function simpleClass(next) {
      var mappingQueries = ['Test', 'class', 'constructor'];
      testJSFormatter('class Test{constructor(){this.bar=10;}givemebar(){return this.bar;}}', mappingQueries, next);
    },

    function extendedClass(next) {
      var mappingQueries = ['Foo', 'Bar', 'extends', 'super', 'name'];
      testJSFormatter(
          'class Foo extends Bar{constructor(name){super(name);}getName(){return super.getName();}}', mappingQueries,
          next);
    },

    function twoConsecutiveClasses(next) {
      var mappingQueries = ['B', 'extends', 'constructor', 'super'];
      testJSFormatter('class A{}class B extends A{constructor(){super();}}', mappingQueries, next);
    },

    function staticMethod(next) {
      var mappingQueries = ['Employer', 'static', '1', 'return'];
      testJSFormatter(
          'class Employer{static count(){this._counter = (this._counter || 0) + 1; return this._counter;}}',
          mappingQueries, next);
    },

    function classExpression(next) {
      var mappingQueries = ['new', 'class', 'constructor', 'debugger'];
      testJSFormatter('new(class{constructor(){debugger}})', mappingQueries, next);
    },
  ]);
})();
