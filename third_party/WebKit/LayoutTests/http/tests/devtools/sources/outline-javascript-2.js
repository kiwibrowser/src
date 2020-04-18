// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Verify javascript outline\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');

  var test = SourcesTestRunner.testJavascriptOutline;

  TestRunner.markStep('testNamedFunctionVariableAssignment');
  await test('var a = function foo(...bar) { }');

  TestRunner.markStep('testArrowFunction');
  await test('var a = x => x + 2');

  TestRunner.markStep('testArrowFunctionWithMultipleArguments');
  await test('var a = (x, y) => x + y');

  TestRunner.markStep('testInnerFunctions');
  await test('function foo(){ function bar() {} function baz() { }}');

  TestRunner.markStep('testObjectProperties');
  await test('x = { run: function() { }, get count() { }, set count(value) { }}');

  TestRunner.completeTest();
})();
