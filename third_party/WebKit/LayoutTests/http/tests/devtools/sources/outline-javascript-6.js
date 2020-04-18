// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Verify javascript outline\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');

  var test = SourcesTestRunner.testJavascriptOutline;

  TestRunner.markStep('testClassAsNumberLiteralProperty');
  await test('var foo = { 42: class { }};');

  TestRunner.markStep('testClassAsStringLiteralProperty');
  await test('var foo = { "foo": class { }};');

  TestRunner.markStep('testClassAsIdentifierProperty');
  await test('var foo = { foo: class { }};');

  TestRunner.completeTest();
})();
