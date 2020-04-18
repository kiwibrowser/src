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
    function testFontFace(next) {
      var css =
          '@font-face{font-family:MyHelvetica;src:local(\'Helvetica Neue Bold\'),local(\'HelveticaNeue-Bold\'),url(MgOpenModernaBold.ttf);font-weight:bold;}div{color:red}';
      var mappingQueries = ['font-face', 'red'];
      testCSSFormatter(css, mappingQueries, next);
    },

    function testCharsetRule(next) {
      var css = '@charset \'iso-8859-15\';p{margin:0}';
      var mappingQueries = ['charset', 'iso', 'margin'];
      testCSSFormatter(css, mappingQueries, next);
    },

    function testImportRule(next) {
      var css = '@import url(\'bluish.css\') projection,tv;span{border:1px solid black}';
      var mappingQueries = ['import', 'bluish', 'projection', 'span', 'border', 'black'];
      testCSSFormatter(css, mappingQueries, next);
    },

    function testImportWithMediaQueryRule(next) {
      var css = '@import url(\'landscape.css\') screen and (orientation:landscape);article{background:yellow}';
      var mappingQueries = ['import', 'url', 'orientation', 'article', 'background', 'yellow'];
      testCSSFormatter(css, mappingQueries, next);
    },

    function testKeyframesRule(next) {
      var css =
          'p{animation-duration:3s;}@keyframes slidein{from{margin-left:100%;width:300%;}to{margin-left:0%;width:100%;}}p{animation-name:slidein}';
      var mappingQueries = ['animation-duration', '3s', 'keyframes', 'from', '300%', 'animation-name'];
      testCSSFormatter(css, mappingQueries, next);
    },
  ]);
})();
