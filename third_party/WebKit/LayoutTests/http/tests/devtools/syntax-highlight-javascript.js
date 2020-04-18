// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that JavaScriptSourceSyntaxHighlighter detects the tokens.\n`);

  function dumpSyntaxHighlightJS(str) {
    return TestRunner.dumpSyntaxHighlight(str, 'text/javascript');
  }

  dumpSyntaxHighlightJS('return\'foo\';');
  dumpSyntaxHighlightJS('/\\\//g');
  dumpSyntaxHighlightJS('//ig\';');
  dumpSyntaxHighlightJS('1 / 2 + /a/.test(\'a\');');
  dumpSyntaxHighlightJS('"\\"/".length / 2');
  dumpSyntaxHighlightJS('var foo = 1/*/***//2');
  dumpSyntaxHighlightJS('/*comment*//.*/.test(\'a\')');
  dumpSyntaxHighlightJS('\'f\\\noo\';');
  dumpSyntaxHighlightJS('\'\\f\\b\\t\';');
  dumpSyntaxHighlightJS('\'/\\\n/\';');
  dumpSyntaxHighlightJS('foo/**\n/\n*/foo');
  dumpSyntaxHighlightJS('{0: true}');
  dumpSyntaxHighlightJS('var toString;');
  dumpSyntaxHighlightJS('var foo = undefined;');
  dumpSyntaxHighlightJS('var foo = Infinity;');
  dumpSyntaxHighlightJS('var foo = NaN;')

      .then(TestRunner.completeTest.bind(TestRunner));
})();
