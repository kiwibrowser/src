// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @typedef {{
 *   fontList: !Array<{
 *       0: string,
 *       1: (string|undefined),
 *       2: (string|undefined)}>,
 *   extensionUrl: string
 * }}
 */
let FontsData;

cr.define('settings', function() {
  /** @interface */
  class FontsBrowserProxy {
    /**
     * @return {!Promise<!FontsData>} Fonts and the advanced font settings
     *     extension URL.
     */
    fetchFontsData() {}

    observeAdvancedFontExtensionAvailable() {}

    openAdvancedFontSettings() {}
  }

  /**
   * @implements {settings.FontsBrowserProxy}
   */
  class FontsBrowserProxyImpl {
    /** @override */
    fetchFontsData() {
      return cr.sendWithPromise('fetchFontsData');
    }

    /** @override */
    observeAdvancedFontExtensionAvailable() {
      chrome.send('observeAdvancedFontExtensionAvailable');
    }

    /** @override */
    openAdvancedFontSettings() {
      chrome.send('openAdvancedFontSettings');
    }
  }

  cr.addSingletonGetter(FontsBrowserProxyImpl);

  return {
    FontsBrowserProxy: FontsBrowserProxy,
    FontsBrowserProxyImpl: FontsBrowserProxyImpl,
  };
});
