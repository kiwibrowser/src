// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`This test checks various Runtime functions.\n`);


  function findLoadedModule() {
    var modules = self.runtime._modules;
    for (var module of modules) {
      if (module._loadedForTest)
        return module;
    }

    TestRunner.addResult('Fail: not a single module loaded');
    TestRunner.completeTest();
  }

  TestRunner.runTestSuite([function substituteURL(next) {
    var module = findLoadedModule();
    var moduleName = module._name;
    module._name = 'fake_module';

    testValue('no url here');
    testValue('@url()');
    testValue('@url(file.js)');
    testValue('before @url(long/path/to/the/file.png) after');
    testValue('@url(first.png)@url(second.gif)');
    testValue('a lot of @url(stuff) in a@url(single)line and more url() @@url (not/a/resource.gif)');

    function testValue(value) {
      TestRunner.addResult('"' + value + '" -> "' + module.substituteURL(value) + '"');
    }

    module._name = moduleName;
    next();
  }]);
})();
