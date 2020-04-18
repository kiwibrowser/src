// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  await TestRunner.setupStartupTest('resources/cached-resource-metadata.html');
  TestRunner.addResult(`Verify that cached resource has metadata.\n`);
  await TestRunner.loadModule('network_test_runner');

  var resource = TestRunner.resourceTreeModel.resourceForURL(
      'http://127.0.0.1:8000/devtools/resource-tree/resources/script-with-constant-last-modified.php');
  if (!resource) {
    TestRunner.addResult('ERROR: Failed to find resource.');
    TestRunner.completeTest();
    return;
  }
  TestRunner.addResult('Last modified: ' + (resource.lastModified() ? resource.lastModified().toISOString() : null));
  TestRunner.addResult('Content size: ' + resource.contentSize());

  TestRunner.completeTest();
})();
