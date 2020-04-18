// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * @suppress {globalThis|missingProperties}
 */

Polymer({
  is: 'passphrase-dialog',
  i18n: function(name) {
    // For tests, chrome.i18n API is not available.
    return chrome.i18n ? chrome.i18n.getMessage(name) : name;
  },

  cancel: function() {
    window.close();
  },

  accept: function() {
    window.onPassphraseSuccess(this.$.input.value);
    window.close();
  },

  ready: function() {
    document.addEventListener('keydown', function(event) {
      if (event.keyCode == 13)  // Enter
        this.$.acceptButton.click();

      if (event.keyCode == 27)  // Escape
        this.$.cancelButton.click();
    }.bind(this));

    // Show the window once ready. Not available for tests.
    if (chrome.app && chrome.app.window)
      chrome.app.window.current().show();

    if (window.onEverythingLoaded)
      window.onEverythingLoaded();
  }
});
