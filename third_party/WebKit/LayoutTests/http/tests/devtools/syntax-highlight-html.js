// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that SourceHTMLTokenizer detects the tokens.\n`);

  function dumpSyntaxHighlightHTML(str) {
    return TestRunner.dumpSyntaxHighlight(str, 'text/html');
  }

  dumpSyntaxHighlightHTML('<html>');
  dumpSyntaxHighlightHTML('<table cellspacing=0>');
  dumpSyntaxHighlightHTML('<input checked value="foo">');
  dumpSyntaxHighlightHTML('<table cellspacing="0" cellpadding=\'0\'>');
  dumpSyntaxHighlightHTML(
      '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">');
  dumpSyntaxHighlightHTML('<!--div><div foobar-->');
  dumpSyntaxHighlightHTML(
      '<script></' +
      'script><!--div-->');
  dumpSyntaxHighlightHTML(
      '<script type="text/javascript">document.write(\'<script type="text/javascript"></\' + \'script>\');</' +
      'script>')

      .then(TestRunner.completeTest.bind(TestRunner));
})();
