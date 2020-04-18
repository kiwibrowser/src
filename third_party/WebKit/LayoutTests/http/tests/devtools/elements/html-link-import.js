// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`This test verifies that imported document is rendered within the import link.\n`);
  await TestRunner.loadModule('elements_test_runner');
  await TestRunner.showPanel('elements');

  await TestRunner.loadHTML(`
  <head>
  <link rel="import" href="../resources/imported-document.html">
  <head>
  `);

  // Warm up highlighter module.
  runtime.loadModulePromise('source_frame').then(function() {
    ElementsTestRunner.expandElementsTree(callback);
  });

  function callback() {
    ElementsTestRunner.dumpElementsTree();
    TestRunner.completeTest();
  }
})();
