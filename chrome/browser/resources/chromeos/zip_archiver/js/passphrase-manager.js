// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @constructor
 */
unpacker.PassphraseManager = function() {};

/**
 * Requests a passphrase from the user.
 * @return {!Promise<string>}
 */
unpacker.PassphraseManager.prototype.getPassphrase = function() {
  return new Promise(function(fulfill, reject) {
    chrome.app.window.create(
        '../html/passphrase.html',
        /** @type {!chrome.app.window.CreateWindowOptions} */ ({
          innerBounds: {width: 320, height: 160},
          alwaysOnTop: true,
          resizable: false,
          frame: 'none',
          hidden: true
        }),
        function(passphraseWindow) {
          var passphraseSucceeded = false;

          passphraseWindow.onClosed.addListener(function() {
            if (passphraseSucceeded)
              return;
            reject('FAILED');
          }.bind(this));

          passphraseWindow.contentWindow.onPassphraseSuccess = function(
                                                                   passphrase) {
            passphraseSucceeded = true;
            fulfill(passphrase);
          }.bind(this);
        }.bind(this));
  }.bind(this));
};
