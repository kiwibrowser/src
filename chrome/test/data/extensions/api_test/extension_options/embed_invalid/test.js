// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function testCannotEmbedExtension(id) {
  var done = chrome.test.callbackAdded();
  var extensionoptions = document.createElement('extensionoptions');
  extensionoptions.addEventListener('createfailed', function() {
    document.body.removeChild(extensionoptions);
    done();
  });
  extensionoptions.addEventListener('load', function () {
    document.body.removeChild(extensionoptions);
    chrome.test.fail();
  });
  extensionoptions.setAttribute('extension', id);
  document.body.appendChild(extensionoptions);
}

chrome.test.runTests([
  function cannotEmbedInvalidExtensionId() {
    testCannotEmbedExtension('thisisprobablynotrealextensionid');
  },

  function cannotEmbedSelfIfNoOptionsPage() {
    testCannotEmbedExtension(chrome.runtime.id);
  }
]);
