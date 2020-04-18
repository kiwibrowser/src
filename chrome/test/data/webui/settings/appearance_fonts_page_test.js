// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @implements {settings.FontsBrowserProxy} */
class TestFontsBrowserProxy extends TestBrowserProxy {
  constructor() {
    super([
      'fetchFontsData',
      'observeAdvancedFontExtensionAvailable',
      'openAdvancedFontSettings',
    ]);

    /** @private {!FontsData} */
    this.fontsData_ = {
      'fontList': [['font name', 'alternate', 'ltr']],
      'encodingList': [['encoding name', 'alternate', 'ltr']],
    };
  }

  /** @override */
  fetchFontsData() {
    this.methodCalled('fetchFontsData');
    return Promise.resolve(this.fontsData_);
  }

  /** @override */
  observeAdvancedFontExtensionAvailable() {
    this.methodCalled('observeAdvancedFontExtensionAvailable');
  }

  /** @override */
  openAdvancedFontSettings() {
    this.methodCalled('openAdvancedFontSettings');
  }
}

let fontsPage = null;

/** @type {?TestFontsBrowserProxy} */
let fontsBrowserProxy = null;

suite('AppearanceFontHandler', function() {
  setup(function() {
    fontsBrowserProxy = new TestFontsBrowserProxy();
    settings.FontsBrowserProxyImpl.instance_ = fontsBrowserProxy;

    PolymerTest.clearBody();

    fontsPage = document.createElement('settings-appearance-fonts-page');
    document.body.appendChild(fontsPage);
  });

  teardown(function() {
    fontsPage.remove();
  });

  test('fetchFontsData', function() {
    return fontsBrowserProxy.whenCalled('fetchFontsData');
  });

  test('openAdvancedFontSettings', function() {
    cr.webUIListenerCallback('advanced-font-settings-installed', [true]);
    Polymer.dom.flush();
    const button = fontsPage.$$('#advancedButton');
    assert(!!button);
    MockInteractions.tap(button);
    return fontsBrowserProxy.whenCalled('openAdvancedFontSettings');
  });
});
