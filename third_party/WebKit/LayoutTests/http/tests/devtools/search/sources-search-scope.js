// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that ScriptSearchScope performs search across all sources correctly. See https://bugs.webkit.org/show_bug.cgi?id=41350\n`);
  await TestRunner.loadModule('application_test_runner');
  await TestRunner.loadModule('network_test_runner');
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');
  await TestRunner.loadHTML(`<iframe src="resources/search.html"></iframe>`);

  var scope = new Sources.SourcesSearchScope();
  await Promise.all([
    TestRunner.waitForUISourceCode('search.html'),
    TestRunner.waitForUISourceCode('search.js'),
    TestRunner.waitForUISourceCode('search.css'),
  ]);

  TestRunner.runTestSuite([
    function testIgnoreCaseAndIgnoreDynamicScript(next) {
      var query = 'searchTest' +
          'UniqueString';
      var searchConfig = new Search.SearchConfig(query, true /* ignoreCase */, false /* isRegex */);
      SourcesTestRunner.runSearchAndDumpResults(scope, searchConfig, next);
    },

    function testIgnoreCase(next) {
      Common.settingForTest('searchInAnonymousAndContentScripts').set(true);
      var query = 'searchTest' +
          'UniqueString';
      var searchConfig = new Search.SearchConfig(query, true /* ignoreCase */, false /* isRegex */);
      SourcesTestRunner.runSearchAndDumpResults(scope, searchConfig, next);
    },

    function testCaseSensitive(next) {
      var query = 'searchTest' +
          'UniqueString';
      var searchConfig = new Search.SearchConfig(query, false /* ignoreCase */, false /* isRegex */);
      SourcesTestRunner.runSearchAndDumpResults(scope, searchConfig, next);
    },

    function testFileHTML(next) {
      var query = 'searchTest' +
          'UniqueString' +
          ' file:html';
      var searchConfig = new Search.SearchConfig(query, true /* ignoreCase */, false /* isRegex */);
      SourcesTestRunner.runSearchAndDumpResults(scope, searchConfig, next);
    },

    function testFileJS(next) {
      var query = 'file:js ' +
          'searchTest' +
          'UniqueString';
      var searchConfig = new Search.SearchConfig(query, true /* ignoreCase */, false /* isRegex */);
      SourcesTestRunner.runSearchAndDumpResults(scope, searchConfig, next);
    },

    function testFileHTMLJS(next) {
      var query = 'file:js ' +
          'searchTest' +
          'UniqueString' +
          ' file:html';
      var searchConfig = new Search.SearchConfig(query, true /* ignoreCase */, false /* isRegex */);
      SourcesTestRunner.runSearchAndDumpResults(scope, searchConfig, next);
    },

    function testSpaceQueries(next) {
      var query = 'searchTest' +
          'Unique' +
          ' space' +
          ' String';
      var searchConfig = new Search.SearchConfig(query, true /* ignoreCase */, false /* isRegex */);
      SourcesTestRunner.runSearchAndDumpResults(scope, searchConfig, next);
    },

    function testSpaceQueriesFileHTML(next) {
      var query = 'file:html ' +
          'searchTest' +
          'Unique' +
          ' space' +
          ' String';
      var searchConfig = new Search.SearchConfig(query, true /* ignoreCase */, false /* isRegex */);
      SourcesTestRunner.runSearchAndDumpResults(scope, searchConfig, next);
    },

    function testSpaceQueriesFileHTML_SEARCH(next) {
      var query = 'file:html ' +
          'searchTest' +
          'Unique' +
          ' space' +
          ' String' +
          ' file:search';
      var searchConfig = new Search.SearchConfig(query, true /* ignoreCase */, false /* isRegex */);
      SourcesTestRunner.runSearchAndDumpResults(scope, searchConfig, next);
    },

    function testSpaceQueriesFileJS_SEARCH_HTML(next) {
      var query = 'file:js ' +
          'searchTest' +
          'Unique' +
          ' space' +
          ' String' +
          ' file:search file:html';
      var searchConfig = new Search.SearchConfig(query, true /* ignoreCase */, false /* isRegex */);
      SourcesTestRunner.runSearchAndDumpResults(scope, searchConfig, next);
    },

    function testSeveralQueriesFileHTML(next) {
      var query = 'searchTest' +
          'Unique' +
          ' file:html' +
          ' space' +
          ' String';
      var searchConfig = new Search.SearchConfig(query, true /* ignoreCase */, false /* isRegex */);
      SourcesTestRunner.runSearchAndDumpResults(scope, searchConfig, next);
    },

    function testSeveralQueriesFileHTML_SEARCH(next) {
      var query = 'searchTest' +
          'Unique' +
          ' file:html' +
          ' space' +
          ' String' +
          ' file:search';
      var searchConfig = new Search.SearchConfig(query, true /* ignoreCase */, false /* isRegex */);
      SourcesTestRunner.runSearchAndDumpResults(scope, searchConfig, next);
    },

    function testSeveralQueriesFileJS_SEARCH_HTML(next) {
      var query = 'file:js ' +
          'searchTest' +
          'Unique' +
          ' file:html' +
          ' space' +
          ' String' +
          ' file:search';
      var searchConfig = new Search.SearchConfig(query, true /* ignoreCase */, false /* isRegex */);
      SourcesTestRunner.runSearchAndDumpResults(scope, searchConfig, next);
    },

    function testSeveralQueriesFileNotCSS(next) {
      var query = 'searchTest' +
          'Unique' +
          ' -file:css' +
          ' space' +
          ' String';
      var searchConfig = new Search.SearchConfig(query, true /* ignoreCase */, false /* isRegex */);
      SourcesTestRunner.runSearchAndDumpResults(scope, searchConfig, next);
    },

    function testFileQueryWithProjectName(next) {
      TestRunner.addResult('Running a file query with existing project name first:');
      var query = 'searchTest' +
          'Unique' +
          ' file:127.0.0.1';
      var searchConfig = new Search.SearchConfig(query, true /* ignoreCase */, false /* isRegex */);
      SourcesTestRunner.runSearchAndDumpResults(scope, searchConfig, step2);

      function step2() {
        TestRunner.addResult('Running a file query with non-existing project name now:');
        query = 'searchTest' +
            'Unique' +
            ' file:128.0.0.1';
        searchConfig = new Search.SearchConfig(query, true /* ignoreCase */, false /* isRegex */);
        SourcesTestRunner.runSearchAndDumpResults(scope, searchConfig, next);
      }
    },

    function testDirtyFiles(next) {
      SourcesTestRunner.showScriptSource('search.js', step2);

      function step2(sourceFrame) {
        sourceFrame.uiSourceCode().setWorkingCopy(
            'FOO ' +
            'searchTest' +
            'UniqueString' +
            ' BAR');
        var query = 'searchTest' +
            'UniqueString';
        var searchConfig = new Search.SearchConfig(query, true /* ignoreCase */, false /* isRegex */);
        SourcesTestRunner.runSearchAndDumpResults(scope, searchConfig, next);
      }
    }
  ]);
})();
