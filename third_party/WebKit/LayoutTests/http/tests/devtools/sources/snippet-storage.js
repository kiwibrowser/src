// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests snippet storage.\n`);
  await TestRunner.showPanel('sources');

  var settingPrefix = 'test';
  var namePrefix = 'Test snippet #';
  var snippetStorage = new Snippets.SnippetStorage(settingPrefix, namePrefix);

  function dumpSnippets(snippets) {
    for (var i = 0; i < snippets.length; ++i) {
      var snippet = snippets[i];
      TestRunner.addResult(
          '    Snippet: id = ' + snippet.id + ', name = \'' + snippet.name + '\', content = \'' + snippet.content +
          '\'.');
    }
  }

  function dumpSavedSnippets() {
    TestRunner.addResult('Dumping saved snippets:');
    dumpSnippets(snippetStorage._snippetsSetting.get());
  }

  function dumpStorageSnippets() {
    TestRunner.addResult('Dumping storage snippets:');
    dumpSnippets(snippetStorage.snippets);
  }

  dumpSavedSnippets();
  dumpStorageSnippets();
  var snippet = snippetStorage.createSnippet();
  TestRunner.addResult('Snippet created.');
  dumpSavedSnippets();
  dumpStorageSnippets();
  snippet.name = 'New snippet name';
  TestRunner.addResult('Snippet renamed.');
  dumpSavedSnippets();
  dumpStorageSnippets();
  snippet.content = 'New snippet content';
  TestRunner.addResult('Snippet content changed.');
  dumpSavedSnippets();
  dumpStorageSnippets();
  var anotherSnippet = snippetStorage.createSnippet();
  TestRunner.addResult('Another snippet created.');
  dumpSavedSnippets();
  dumpStorageSnippets();
  snippetStorage.deleteSnippet(snippet);
  TestRunner.addResult('Snippet deleted.');
  dumpSavedSnippets();
  dumpStorageSnippets();
  snippetStorage.deleteSnippet(anotherSnippet);
  TestRunner.addResult('Another snippet deleted.');
  dumpSavedSnippets();
  dumpStorageSnippets();

  TestRunner.completeTest();
})();
