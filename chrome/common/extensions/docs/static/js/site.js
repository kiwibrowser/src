// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

function addGplusButton() {
  var po = document.createElement('script'); po.type = 'text/javascript'; po.async = true;
  po.src = 'https://apis.google.com/js/plusone.js?onload=onLoadCallback';
  var s = document.getElementsByTagName('script')[0]; s.parentNode.insertBefore(po, s);
}

function openFeedback(e) {
  e.preventDefault();
  userfeedback.api.startFeedback({productId: 86265});
}

function addGoogleFeedback() {
  [].forEach.call(document.querySelectorAll('[data-feedback]'), function(el, i) {
    el.addEventListener('click', openFeedback);
  });
}


// Auto syntax highlight all pre tags.
function prettyPrintCode() {
  var pres = document.querySelectorAll('pre');
  for (var i = 0, pre; pre = pres[i]; ++i) {
    pre.classList.add('prettyprint');
  }
  window.prettyPrint && prettyPrint();
}

prettyPrintCode();
addGoogleFeedback();
addGplusButton();

})();