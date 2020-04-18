// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('settings_default_browser', function() {
  /**
   * A test version of DefaultBrowserBrowserProxy. Provides helper methods
   * for allowing tests to know when a method was called, as well as
   * specifying mock responses.
   *
   * @implements {settings.DefaultBrowserBrowserProxy}
   */
  class TestDefaultBrowserBrowserProxy extends TestBrowserProxy {
    constructor() {
      super([
        'requestDefaultBrowserState',
        'setAsDefaultBrowser',
      ]);

      /** @private {!DefaultBrowserInfo} */
      this.defaultBrowserInfo_ = {
        canBeDefault: true,
        isDefault: false,
        isDisabledByPolicy: false,
        isUnknownError: false
      };
    }

    /** @override */
    requestDefaultBrowserState() {
      this.methodCalled('requestDefaultBrowserState');
      return Promise.resolve(this.defaultBrowserInfo_);
    }

    /** @override */
    setAsDefaultBrowser() {
      this.methodCalled('setAsDefaultBrowser');
    }

    /**
     * Sets the response to be returned by |requestDefaultBrowserState|.
     * @param {!DefaultBrowserInfo} info Fake info for testing.
     */
    setDefaultBrowserInfo(info) {
      this.defaultBrowserInfo_ = info;
    }
  }

  suite('DefaultBrowserPageTest', function() {
    let page = null;

    /** @type {?settings.TestDefaultBrowserBrowserProxy} */
    let browserProxy = null;

    setup(function() {
      browserProxy = new TestDefaultBrowserBrowserProxy();
      settings.DefaultBrowserBrowserProxyImpl.instance_ = browserProxy;
      return initPage();
    });

    teardown(function() {
      page.remove();
      page = null;
    });

    /** @return {!Promise} */
    function initPage() {
      browserProxy.reset();
      PolymerTest.clearBody();
      page = document.createElement('settings-default-browser-page');
      document.body.appendChild(page);
      return browserProxy.whenCalled('requestDefaultBrowserState');
    }

    test('default-browser-test-can-be-default', function(done) {
      browserProxy.setDefaultBrowserInfo({
        canBeDefault: true,
        isDefault: false,
        isDisabledByPolicy: false,
        isUnknownError: false
      });

      return initPage().then(function() {
        assertFalse(page.isDefault_);
        assertFalse(page.isSecondaryInstall_);
        assertFalse(page.isUnknownError_);
        assertTrue(page.maySetDefaultBrowser_);
        done();
      });
    });

    test('default-browser-test-is-default', function(done) {
      assertTrue(!!page);
      browserProxy.setDefaultBrowserInfo({
        canBeDefault: true,
        isDefault: true,
        isDisabledByPolicy: false,
        isUnknownError: false
      });

      return initPage().then(function() {
        assertTrue(page.isDefault_);
        assertFalse(page.isSecondaryInstall_);
        assertFalse(page.isUnknownError_);
        assertFalse(page.maySetDefaultBrowser_);
        done();
      });
    });

    test('default-browser-test-is-secondary-install', function(done) {
      browserProxy.setDefaultBrowserInfo({
        canBeDefault: false,
        isDefault: false,
        isDisabledByPolicy: false,
        isUnknownError: false
      });

      return initPage().then(function() {
        assertFalse(page.isDefault_);
        assertTrue(page.isSecondaryInstall_);
        assertFalse(page.isUnknownError_);
        assertFalse(page.maySetDefaultBrowser_);
        done();
      });
    });

    test('default-browser-test-is-disabled-by-policy', function(done) {
      browserProxy.setDefaultBrowserInfo({
        canBeDefault: true,
        isDefault: false,
        isDisabledByPolicy: true,
        isUnknownError: false
      });

      return initPage().then(function() {
        assertFalse(page.isDefault_);
        assertFalse(page.isSecondaryInstall_);
        assertTrue(page.isUnknownError_);
        assertFalse(page.maySetDefaultBrowser_);
        done();
      });
    });

    test('default-browser-test-is-unknown-error', function(done) {
      browserProxy.setDefaultBrowserInfo({
        canBeDefault: true,
        isDefault: false,
        isDisabledByPolicy: false,
        isUnknownError: true
      });

      return initPage().then(function() {
        assertFalse(page.isDefault_);
        assertFalse(page.isSecondaryInstall_);
        assertTrue(page.isUnknownError_);
        assertFalse(page.maySetDefaultBrowser_);
        done();
      });
    });
  });
});
