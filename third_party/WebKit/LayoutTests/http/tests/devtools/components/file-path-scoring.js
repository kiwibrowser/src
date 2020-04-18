// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Test file path scoring function\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.addScriptTag('../resources/example-fileset-for-test.js');

  const TestQueries = [
    ['textepl', './Source/devtools/front_end/TextEditor.pl'],
    ['defted', './Source/devtools/front_end/DefaultTextEditor.pl'],
    ['CMTE', './Source/devtools/front_end/CodeMirrorTextEditor.pl'],
    ['frocmte', './Source/devtools/front_end/CodeMirrorTextEditor.pl'],
    ['cmtepl', './Source/devtools/front_end/CodeMirrorTextEditor.pl'],
    ['setscr', './Source/devtools/front_end/SettingsScreen.pl'],
    ['cssnfv', './Source/devtools/front_end/CSSNamedFlowView.pl'],
    ['jssf', './Source/devtools/front_end/JavaScriptSourceFrame.pl'],
    ['sofrapl', './Source/devtools/front_end/SourceFrame.pl'],
    ['inspeins', './Source/core/inspector/InspectorInstrumentation.z'],
    ['froscr', './Source/devtools/front_end/Script.pl'],
    ['adscon', './Source/devtools/front_end/AdvancedSearchController.pl']
  ];
  TestRunner.evaluateInPage('blinkFilePaths()', step1);
  TestRunner.addResult('Expected score must be equal to the actual score');

  function step1(filePaths) {
    var files = filePaths.split(':');
    TestRunner.addResult('Test set size: ' + files.length);
    for (var i = 0; i < TestQueries.length; ++i) {
      runQuery(files, TestQueries[i][0], TestQueries[i][1]);
    }

    runQuery(
        ['svg/SVGTextRunRenderingContext.cpp', 'execution_context/ExecutionContext.cpp', 'testing/NullExecutionContext.cpp'],
        'execontext', 'execution_context/ExecutionContext.cpp');

    TestRunner.completeTest();
  }

  function runQuery(paths, query, expected) {
    var scorer = new Sources.FilePathScoreFunction(query);
    var bestScore = -1;
    var bestIndex = -1;
    var filter = String.filterRegex(query);
    for (var i = 0; i < paths.length; ++i) {
      if (!filter.test(paths[i]))
        continue;
      var score = scorer.score(paths[i]);
      if (score > bestScore) {
        bestScore = score;
        bestIndex = i;
      }
    }
    var result = '\n=== Test query: ' + query;
    result += '\n    expected return: ' + render(scorer, expected);
    result += '\n      actual return: ' + render(scorer, paths[bestIndex]);
    TestRunner.addResult(result);
  }

  function render(scorer, value) {
    var indexes = [];
    var score = scorer.score(value, indexes);
    var result = '';
    var bold = false;
    for (var i = 0; i < value.length; ++i) {
      if (indexes.indexOf(i) !== -1) {
        if (!bold)
          result += '>';
        bold = true;
        result += value[i];
      } else {
        if (bold)
          result += '<';
        result += value[i];
        bold = false;
      }
    }
    if (bold)
      result += '<';
    return result + ' (score: ' + score + ')';
  }
})();
