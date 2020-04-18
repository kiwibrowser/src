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
    function testSimpleCSS(next) {
      var content = 'a { /* pre-comment */ color /* after name */ : /* before value */ red /* post-comment */ }';
      testCSSFormatter(content, ['pre-comment', 'post-comment'], next);
    },

    function testComplexCSS(next) {
      SourcesTestRunner.showScriptSource('style-formatter-obfuscated.css', didShowScriptSource);

      function didShowScriptSource(sourceFrame) {
        var mappingQueries = [
          '@media',
          'screen',
          'html',
          'color',
          'green',
          'foo-property',
          'bar-value',
          'body',
          'background',
          'black',
        ];
        testCSSFormatter(sourceFrame._textEditor.text(), mappingQueries, next);
      }
    },

    function testFormatInlinedStyles(next) {
      var content =
          '<html><body><style>@-webkit-keyframes{from{left: 0} to{left:100px;}}</style><style>badbraces { }} @media screen{a.b{color:red;text-decoration: none}}</style></body></html>';
      SourcesTestRunner.testPrettyPrint('text/html', content, [], next);
    },

    function testNonZeroLineMapping(next) {
      var mappingQueries = ['div', 'color', 'red'];
      testCSSFormatter('\n\ndiv { color: red; }', mappingQueries, next);
    },

    function testComplexSelector(next) {
      var css = 'a.b.c:hover,.d.e.f.g::before,h.i{color:red;}';
      var mappingQueries = ['a', '.b', '.c', '.d', '.e', '.f', '.g', 'h', '.i', 'color', 'red'];
      testCSSFormatter(css, mappingQueries, next);
    },
  ]);
})();
