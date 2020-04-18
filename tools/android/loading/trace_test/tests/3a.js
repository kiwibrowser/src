/* Copyright 2016 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

function addImg(img_link) {
  var img = document.createElement('img');
  img.setAttribute('src', img_link);
  img.setAttribute('alt', '');
  document.body.appendChild(img);
}

function fn1() {
  addImg('3a.jpg');
}

var scr = document.createElement('script');
scr.setAttribute('src', '3b.js');
scr.setAttribute('type', 'text/javascript');
document.getElementsByTagName('head')[0].insertBefore(
    scr, document.getElementsByTagName('script')[0].nextSibling);
