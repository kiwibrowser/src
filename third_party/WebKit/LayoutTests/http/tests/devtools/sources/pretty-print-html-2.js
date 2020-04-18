// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Verifies JavaScript pretty-printing functionality.\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');

  var testFormatter = SourcesTestRunner.testPrettyPrint.bind(SourcesTestRunner, 'text/html');

  TestRunner.runTestSuite([
    function testDocType(next) {
      var mappingQueries = ['<body>', 'hello', '</body>'];
      testFormatter('<!DOCTYPE HTML><body>hello, world</body>', mappingQueries, next);
    },

    function testComment(next) {
      var mappingQueries = ['<body>', 'comment 1', 'comment 2', 'comment 3', 'link'];
      testFormatter(
          '<!-- comment 1 --><html><!-- comment 2--><meta/><body><!-- comment 3--><a>link</a></body></html>',
          mappingQueries, next);
    },

    function testNonJavascriptScriptTag(next) {
      var mappingQueries = ['type', 'R', '</div>', '<\/script>'];
      testFormatter('<div><script type=\'text/K\'>2_&{&/x!/:2_!x}\'!R<\/script></div>', mappingQueries, next);
    },

    function testList(next) {
      var mappingQueries = ['foo', 'bar', 'baz', 'hello', 'world', 'another'];
      testFormatter(
          '<ul><li>foo<li> hello <b>world</b>!<li> hello <b>world</b> <b>i\'m here</b><li>bar<li>baz<li>hello <b>world</b><li>another</ul>',
          mappingQueries, next);
    },

    function testAutomaticClosingTags(next) {
      var mappingQueries = ['aaaa', 'bbbb1', 'bbbb2', 'cccc', 'dddd'];
      testFormatter('<a>aaaa<b>bbbb1<c>cccc<d>dddd</c>bbbb2</a>', mappingQueries, next);
    },
  ]);
})();
