// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  await TestRunner.setupStartupTest('resources/resource-tree-mimetype.html');
  TestRunner.addResult(
      `Tests that resources panel correctly shows mime type when it loads data from memory cache. https://bugs.webkit.org/show_bug.cgi?id=63701\n`);
  await TestRunner.loadModule('application_test_runner');

  function format(resource) {
    return resource.resourceType().name() + ' ' + resource.mimeType + ' ' + resource.url;
  }

  ApplicationTestRunner.dumpResources(format);
  TestRunner.completeTest();
})();
