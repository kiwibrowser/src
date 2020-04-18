// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('settings', function() {
  /** @implements {settings.LanguagesBrowserProxy} */
  class TestLanguagesBrowserProxy extends TestBrowserProxy {
    constructor() {
      const methodNames = [];
      if (cr.isChromeOS || cr.isWindows)
        methodNames.push('getProspectiveUILanguage');

      super(methodNames);

      /** @private {!LanguageSettingsPrivate} */
      this.languageSettingsPrivate_ =
          new settings.FakeLanguageSettingsPrivate();

      /** @private {!InputMethodPrivate} */
      this.inputMethodPrivate_ = new settings.FakeInputMethodPrivate();
    }

    /** @override */
    getLanguageSettingsPrivate() {
      return this.languageSettingsPrivate_;
    }

    /** @override */
    getInputMethodPrivate() {
      return this.inputMethodPrivate_;
    }
  }

  if (cr.isChromeOS || cr.isWindows) {
    /** @override */
    TestLanguagesBrowserProxy.prototype.getProspectiveUILanguage = function() {
      this.methodCalled('getProspectiveUILanguage');
      return Promise.resolve('en-US');
    };
  }

  return {
    TestLanguagesBrowserProxy: TestLanguagesBrowserProxy,
  };
});
