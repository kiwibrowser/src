// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Verifies CSS pretty-printing functionality.\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');
  await TestRunner.addStylesheetTag('resources/style-formatter-obfuscated.css');

  var testCSSFormatter = SourcesTestRunner.testPrettyPrint.bind(SourcesTestRunner, 'text/css');

  TestRunner.runTestSuite([
    function testMediaRule(next) {
      var css = '@media screen,print{body{line-height:1.2}}span{line-height:10px}';
      var mappingQueries = ['@media', 'screen', 'print', 'body', 'line-height', '1.2'];
      testCSSFormatter(css, mappingQueries, next);
    },

    function testNamespaceRule(next) {
      var css = '@namespace svg url(http://www.w3.org/2000/svg);g{color:red}';
      var mappingQueries = ['namespace', 'url', 'color', 'red'];
      testCSSFormatter(css, mappingQueries, next);
    },

    function testPageRule(next) {
      var css = '@page :first{margin:2in 3in;}span{color:blue}';
      var mappingQueries = ['page', 'first', 'margin', '3in'];
      testCSSFormatter(css, mappingQueries, next);
    },

    function testSupportsRule(next) {
      var css = '@supports(--foo:green){body{color:green;}}#content{font-size:14px}';
      var mappingQueries = ['supports', 'foo', 'body', 'color'];
      testCSSFormatter(css, mappingQueries, next);
    },

    function testViewportRule(next) {
      var css = '@viewport{zoom:0.75;min-zoom:0.5;max-zoom:0.9;}footer{position:fixed;bottom:0;}';
      var mappingQueries = ['viewport', 'zoom', '0.5', '0.9'];
      testCSSFormatter(css, mappingQueries, next);
    },
  ]);
})();
