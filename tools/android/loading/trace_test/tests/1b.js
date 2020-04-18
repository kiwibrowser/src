/* Copyright 2016 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Droid Sans from Google Fonts */
var font = '@font-face { font-family: "inline"; ' +
    'src: url(data:application/font-woff2;charset=utf-8;base64,' +
    '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) ' +
    'format("woff2"); font-weight: normal; font-style: normal;} ' +
    '.inside { font-family: test1; color: green;}';

var sty = document.createElement('style');
sty.appendChild(document.createTextNode(font));
document.getElementsByTagName('head')[0].appendChild(sty);

var dummyToCheckStackIsnotJustEndOfFile = 0;
function makingTheStackTraceReallyInteresting(x) {
  dummyToCheckStackIsnotJustEndOfFile = x + 3;
}
makingTheStackTraceReallyInteresting(5);
