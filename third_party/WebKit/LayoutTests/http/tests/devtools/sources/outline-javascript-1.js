// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Verify javascript outline\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');

  var test = SourcesTestRunner.testJavascriptOutline;

  TestRunner.markStep('testSimpleFunction');
  await test('function foo(a, b, c) {}');

  TestRunner.markStep('testSpreadOperator');
  await test('function foo(a, b, ...c) {}');

  TestRunner.markStep('testVariableDeclaration');
  await test('var a = function(a,b) { }');

  TestRunner.markStep('testMultipleVariableDeclaration');
  await test('var a = function(a,b) { }, b = function(c,d) { }');

  TestRunner.markStep('testObjectProperty');
  await test('a.b.c = function(d, e) { }');

  TestRunner.completeTest();
})();
