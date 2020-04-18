// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Verify javascript outline\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');

  var test = SourcesTestRunner.testJavascriptOutline;

  TestRunner.markStep('testClassConstructor');
  await test('class Test { constructor(foo, bar) { }}');

  TestRunner.markStep('testClassMethods');
  await test('class Test { foo() {} bar() { }}');

  TestRunner.markStep('testAnonymousClass');
  await test('var test = class { constructor(foo, bar) { }}');

  TestRunner.markStep('testClassExtends');
  await test('var A = class extends B { foo() { }}');

  TestRunner.markStep('testStaticMethod');
  await test('class Test { static foo() { }}');

  TestRunner.completeTest();
})();
