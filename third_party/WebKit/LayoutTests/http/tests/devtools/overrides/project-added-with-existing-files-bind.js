// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Ensures that when a project is added with already existing files they bind.\n`);
  await TestRunner.loadModule('bindings_test_runner');

  await TestRunner.navigatePromise('http://127.0.0.1:8000/devtools/network/resources/empty.html');

  var bindings = /** @type {!Array<!Persistence.PersistenceBinding>} */ ([]);
  Persistence.persistence.addEventListener(Persistence.Persistence.Events.BindingCreated, event => bindings.push(event.data));

  var {project, testFileSystem} = await BindingsTestRunner.createOverrideProject('file:///tmp');
  testFileSystem.addFile('127.0.0.1%3a8000/devtools/network/resources/empty.html', 'New Content');

  BindingsTestRunner.setOverridesEnabled(true);

  TestRunner.addResult('Bound Files:');
  for (var binding of bindings)
    TestRunner.addResult(binding.network.url() + ' <=> ' + binding.fileSystem.url());
  TestRunner.addResult('');

  TestRunner.completeTest();
})();
