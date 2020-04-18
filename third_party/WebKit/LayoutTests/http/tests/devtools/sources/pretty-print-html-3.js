// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Verifies JavaScript pretty-printing functionality.\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');

  var testFormatter = SourcesTestRunner.testPrettyPrint.bind(SourcesTestRunner, 'text/html');

  TestRunner.runTestSuite([
    function testLinkFollowedByComment(next) {
      var mappingQueries = ['stylesheet', 'some', 'comment'];
      testFormatter('<link href=\'a/b/c.css\' rel=\'stylesheet\'><!-- some comment -->', mappingQueries, next);
    },

    function testInlineJavascript(next) {
      var mappingQueries = ['console', 'test', '</html'];
      testFormatter(
          '<html><script type="text/javascript">for(var i=0;i<10;++i)console.log(\'test \'+i);<\/script></html>',
          mappingQueries, next);
    },

    function testInlineCSS(next) {
      var mappingQueries = ['<html>', 'red', 'black'];
      testFormatter('<html><style>div{color:red;border:1px solid black;}</style></html>', mappingQueries, next);
    },

    function testMultilineInput(next) {
      var html = `<html>
<head>
<meta name=\"ROBOTS\" content=\"NOODP\">
<meta name='viewport' content='text/html'>
<title>foobar</title>
<body>
<script>if(1<2){if(2<3){if(3<4){if(4<5){console.log("magic")}}}}<\/script>
<img src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADIA...">
<style>div{display:flex;align-items:center;justify-content:center;}body{width:100%}*{border:1px solid black}</style>
</body>
</html>
`;
      var mappingQueries = ['ROBOTS', 'image', '...', '</body>', '</html>', '</style>'];
      testFormatter(html, mappingQueries, next);
    },
  ]);
})();
