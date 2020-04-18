// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Verifies JavaScript pretty-printing functionality.\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');

  var testFormatter = SourcesTestRunner.testPrettyPrint.bind(SourcesTestRunner, 'text/html');

  TestRunner.runTestSuite([
    function simpleHTML(next) {
      var mappingQueries = ['</head>', 'test', '</title>'];
      testFormatter('<html><head><title>test</title></head></html>', mappingQueries, next);
    },

    function selfClosingTags(next) {
      var mappingQueries = ['meta', 'hr', '<html>', '</html>'];
      testFormatter('<html><head><meta></head><img><hr/></html>', mappingQueries, next);
    },

    function erroneousSelfClosingTags(next) {
      var mappingQueries = ['<br/>', '<title>', 'test', '</head>'];
      testFormatter('<head><meta><meta></meta><br/></br><link></link><title>test</title></head>', mappingQueries, next);
    },

    function testAttributes(next) {
      var mappingQueries = ['<body>', 'width', 'height', '</body>'];
      testFormatter(
          '<body><canvas width=100 height=100 data-bad-attr=\'</canvas>\'></canvas></body>', mappingQueries, next);
    },

    function testCustomElements(next) {
      var mappingQueries = ['<body>', 'custom-time', 'year', 'month', '</body>'];
      testFormatter(
          '<body><custom-time year=2016 day=1 month=1><div>minutes/seconds</div></custom-time></body>', mappingQueries,
          next);
    }
  ]);
})();
