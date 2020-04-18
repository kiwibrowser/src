// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  await TestRunner.setupStartupTest('resources/main-resource-content.html');
  TestRunner.addResult(`Tests main resource content is correctly loaded and decoded using correct encoding.\n`);
  await TestRunner.loadModule('application_test_runner');

  ApplicationTestRunner.runAfterResourcesAreFinished(
      ['main-resource-content-frame-utf8.php', 'main-resource-content-frame.html'], step2);

  async function step2() {
    TestRunner.addResult('Requesting content: ');
    var resource = ApplicationTestRunner.resourceMatchingURL('main-resource-content-frame.html');
    var content = await TestRunner.PageAgent.getResourceContent(resource.frameId, resource.url);

    TestRunner.assertTrue(!!content, 'No content available.');
    TestRunner.addResult('Resource url: ' + resource.url);
    TestRunner.addResult('Resource content: ' + content);

    TestRunner.addResult('Requesting utf8 content: ');
    resource = ApplicationTestRunner.resourceMatchingURL('main-resource-content-frame-utf8.php');
    content = await TestRunner.PageAgent.getResourceContent(resource.frameId, resource.url);

    TestRunner.assertTrue(!!content, 'No content available.');
    TestRunner.addResult('Resource url: ' + resource.url);
    TestRunner.addResult('Resource content: ' + content);
    TestRunner.completeTest();
  }
})();
