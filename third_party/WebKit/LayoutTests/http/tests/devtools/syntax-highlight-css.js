// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that (S)CSS syntax highlighter properly detects the tokens.\n`);

  function dumpSyntaxHighlightSCSS(str) {
    return TestRunner.dumpSyntaxHighlight(str, 'text/x-scss');
  }

  dumpSyntaxHighlightSCSS('a[href=\'/\']');
  dumpSyntaxHighlightSCSS('#content > a:hover');
  dumpSyntaxHighlightSCSS('@import url(style.css);');
  dumpSyntaxHighlightSCSS('@import url("style.css") projection, tv;');
  dumpSyntaxHighlightSCSS('@import "/css/fireball_unicode.css"; html {}');
  dumpSyntaxHighlightSCSS('@media screen { body { color: red; } }');
  dumpSyntaxHighlightSCSS('@font-face { font-family: "MyHelvetica"; }');
  dumpSyntaxHighlightSCSS(
      'p { color: color; red: red; color: #000; color: #FFF; color: #123AbC; color: #faebfe; color:papayawhip; }');
  dumpSyntaxHighlightSCSS('p { margin: -10px !important; }');
  dumpSyntaxHighlightSCSS('$margin-left: $offsetBefore + 12px + $offsetAfter;');
  dumpSyntaxHighlightSCSS(
      '$type: monster;\n' +
      'p {\n' +
      '@if $type == ocean {\n' +
      'color: blue;\n' +
      '} @else if $type == matador {\n' +
      'color: red;\n' +
      '} @else if $type == monster {\n' +
      'color: green;\n' +
      '} @else {\n' +
      'color: black;\n' +
      '}\n' +
      '}');
  dumpSyntaxHighlightSCSS('@for $i from 1 through 3 { .item-#{$i} { width: 2em * $i; } }');
  dumpSyntaxHighlightSCSS(
      '@mixin adjust-location($x, $y) {\n' +
      '@if unitless($x) {\n' +
      '@warn "Assuming #{$x} to be in pixels";\n' +
      '$x: 1px * $x;\n' +
      '}\n' +
      'position: relative; left: $x; top: $y;\n' +
      '}');

  dumpSyntaxHighlightSCSS(
      '#navbar {\n' +
      '$navbar-width: 800px;\n' +
      '$items: 5;\n' +
      '$navbar-color: #ce4dd6;\n' +

      'width: $navbar-width;\n' +
      'border-bottom: 2px solid $navbar-color;\n' +

      'li {\n' +
      '@extend .notice !optional;\n' +
      'float: left;\n' +
      'width: $navbar-width/$items - 10px;\n' +
      'background-color: lighten($navbar-color, 20%);\n' +
      '&:hover {\n' +
      'background-color: lighten($navbar-color, 10%);\n' +
      '}\n' +
      '}\n' +
      '}')

      .then(TestRunner.completeTest.bind(TestRunner));
})();
