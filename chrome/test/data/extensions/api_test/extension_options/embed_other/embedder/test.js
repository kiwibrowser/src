// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.test.runTests([
  function cannotEmbedOtherExtensionsOptions() {
    var pass = chrome.test.callbackPass;
    chrome.storage.local.get('embeddedId', pass(function(items) {
      var done = chrome.test.callbackAdded();
      var extensionoptions = document.createElement('extensionoptions');
      extensionoptions.addEventListener('createfailed', function() {
        try {
          chrome.runtime.sendMessage(items['embeddedId'],
                                     'checking for options page',
                                     function(response) {
            chrome.test.assertFalse(response.hasOptionsPage);
            done();
          });
        } finally {
          document.body.removeChild(extensionoptions);
        }
      });
      extensionoptions.addEventListener('load', function () {
        document.body.removeChild(extensionoptions);
        chrome.test.fail();
      });
      extensionoptions.setAttribute('extension', items.embeddedId);
      document.body.appendChild(extensionoptions);
    }));
  }
]);
