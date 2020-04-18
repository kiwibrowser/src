// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

window.addEventListener('message', function(e) {
  var response = '';
  var data = JSON.parse(e.data);

  if (data[0] == 'visibilityState-request') {
    response = document.visibilityState;
  } else {
    response = 'FAILED';
  }

  e.source.postMessage(
      JSON.stringify(['visibilityState-response', response]), '*');
});
