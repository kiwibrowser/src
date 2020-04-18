// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('settings_privacy_page', function() {
  /** @implements {settings.ClearBrowsingDataBrowserProxy} */
  class TestClearBrowsingDataBrowserProxy extends TestBrowserProxy {
    constructor() {
      super(['initialize', 'clearBrowsingData', 'getImportantSites']);

      /**
       * The promise to return from |clearBrowsingData|.
       * Allows testing code to test what happens after the call is made, and
       * before the browser responds.
       * @private {?Promise}
       */
      this.clearBrowsingDataPromise_ = null;

      /**
       * Response for |getImportantSites|.
       * @private {!Array<!ImportantSite>}
       */
      this.importantSites_ = [];
    }

    /** @param {!Promise} promise */
    setClearBrowsingDataPromise(promise) {
      this.clearBrowsingDataPromise_ = promise;
    }

    /** @override */
    clearBrowsingData(dataTypes, timePeriod, importantSites) {
      this.methodCalled(
          'clearBrowsingData', [dataTypes, timePeriod, importantSites]);
      cr.webUIListenerCallback('browsing-data-removing', true);
      return this.clearBrowsingDataPromise_ !== null ?
          this.clearBrowsingDataPromise_ :
          Promise.resolve();
    }

    /** @param {!Array<!ImportantSite>} sites */
    setImportantSites(sites) {
      this.importantSites_ = sites;
    }

    /** @override */
    getImportantSites() {
      this.methodCalled('getImportantSites');
      return Promise.resolve(this.importantSites_);
    }

    /** @override */
    initialize() {
      this.methodCalled('initialize');
      return Promise.resolve(false);
    }
  }

  function getClearBrowsingDataPrefs() {
    return {
      browser: {
        clear_data: {
          time_period: {
            key: 'browser.clear_data.time_period',
            type: chrome.settingsPrivate.PrefType.NUMBER,
            value: 0,
          },
          time_period_basic: {
            key: 'browser.clear_data.time_period_basic',
            type: chrome.settingsPrivate.PrefType.NUMBER,
            value: 0,
          },
          browsing_history: {
            key: 'browser.clear_data.browsing_history',
            type: chrome.settingsPrivate.PrefType.BOOLEAN,
            value: false,
          },
          cookies: {
            key: 'browser.clear_data.cookies',
            type: chrome.settingsPrivate.PrefType.BOOLEAN,
            value: false,
          },
          cookies_basic: {
            key: 'browser.clear_data.cookies_basic',
            type: chrome.settingsPrivate.PrefType.BOOLEAN,
            value: false,
          },
          cache_basic: {
            key: 'browser.clear_data.cache_basic',
            type: chrome.settingsPrivate.PrefType.BOOLEAN,
            value: false,
          },
        },
        last_clear_browsing_data_tab: {
          key: 'browser.last_clear_browsing_data_tab',
          type: chrome.settingsPrivate.PrefType.NUMBER,
          value: 0,
        },
      }
    };
  }

  function registerNativeCertificateManagerTests() {
    suite('NativeCertificateManager', function() {
      /** @type {settings.TestPrivacyPageBrowserProxy} */
      let testBrowserProxy;

      /** @type {SettingsPrivacyPageElement} */
      let page;

      setup(function() {
        testBrowserProxy = new TestPrivacyPageBrowserProxy();
        settings.PrivacyPageBrowserProxyImpl.instance_ = testBrowserProxy;
        PolymerTest.clearBody();
        page = document.createElement('settings-privacy-page');
        document.body.appendChild(page);
      });

      teardown(function() {
        page.remove();
      });

      test('NativeCertificateManager', function() {
        MockInteractions.tap(page.$$('#manageCertificates'));
        return testBrowserProxy.whenCalled('showManageSSLCertificates');
      });
    });
  }

  function registerPrivacyPageTests() {
    suite('PrivacyPage', function() {
      /** @type {SettingsPrivacyPageElement} */
      let page;

      setup(function() {
        page = document.createElement('settings-privacy-page');
        document.body.appendChild(page);
      });

      teardown(function() {
        page.remove();
      });

      test('showClearBrowsingDataDialog', function() {
        assertFalse(!!page.$$('settings-clear-browsing-data-dialog'));
        MockInteractions.tap(page.$$('#clearBrowsingData'));
        Polymer.dom.flush();

        const dialog = page.$$('settings-clear-browsing-data-dialog');
        assertTrue(!!dialog);

        // Ensure that the dialog is fully opened before returning from this
        // test, otherwise asynchronous code run in attached() can cause flaky
        // errors.
        return test_util.whenAttributeIs(
            dialog.$$('#clearBrowsingDataDialog'), 'open', '');
      });
    });
  }

  function registerClearBrowsingDataTests() {
    suite('ClearBrowsingData', function() {
      /** @type {settings.TestClearBrowsingDataBrowserProxy} */
      let testBrowserProxy;

      /** @type {SettingsClearBrowsingDataDialogElement} */
      let element;

      setup(function() {
        testBrowserProxy = new TestClearBrowsingDataBrowserProxy();
        settings.ClearBrowsingDataBrowserProxyImpl.instance_ = testBrowserProxy;
        PolymerTest.clearBody();
        element = document.createElement('settings-clear-browsing-data-dialog');
        element.set('prefs', getClearBrowsingDataPrefs());
        document.body.appendChild(element);
        return testBrowserProxy.whenCalled('initialize');
      });

      teardown(function() {
        element.remove();
      });

      test('ClearBrowsingDataTap', function() {
        assertTrue(element.$$('#clearBrowsingDataDialog').open);
        assertFalse(element.showImportantSitesDialog_);

        const cancelButton = element.$$('.cancel-button');
        assertTrue(!!cancelButton);
        const actionButton = element.$$('.action-button');
        assertTrue(!!actionButton);
        const spinner = element.$$('paper-spinner-lite');
        assertTrue(!!spinner);

        // Select a datatype for deletion to enable the clear button.
        const cookieCheckbox = element.$$('#cookiesCheckboxBasic');
        assertTrue(!!cookieCheckbox);
        MockInteractions.tap(cookieCheckbox.$.checkbox);

        assertFalse(cancelButton.disabled);
        assertFalse(actionButton.disabled);
        assertFalse(spinner.active);

        const promiseResolver = new PromiseResolver();
        testBrowserProxy.setClearBrowsingDataPromise(promiseResolver.promise);
        MockInteractions.tap(actionButton);

        return testBrowserProxy.whenCalled('clearBrowsingData')
            .then(function([dataTypes, timePeriod, importantSites]) {
              assertEquals(1, dataTypes.length);
              assertEquals('browser.clear_data.cookies_basic', dataTypes[0]);
              assertTrue(element.$$('#clearBrowsingDataDialog').open);
              assertTrue(cancelButton.disabled);
              assertTrue(actionButton.disabled);
              assertTrue(spinner.active);
              assertTrue(importantSites.length == 0);

              // Simulate signal from browser indicating that clearing has
              // completed.
              cr.webUIListenerCallback('browsing-data-removing', false);
              promiseResolver.resolve();
              // Yields to the message loop to allow the callback chain of the
              // Promise that was just resolved to execute before the
              // assertions.
            })
            .then(function() {
              assertFalse(element.$$('#clearBrowsingDataDialog').open);
              assertFalse(cancelButton.disabled);
              assertFalse(actionButton.disabled);
              assertFalse(spinner.active);
              assertFalse(!!element.$$('#notice'));
              // Check that the dialog didn't switch to important sites.
              assertFalse(element.showImportantSitesDialog_);
            });
      });

      test('ClearBrowsingDataClearButton', function() {
        assertTrue(element.$$('#clearBrowsingDataDialog').open);

        const actionButton = element.$$('.action-button');
        assertTrue(!!actionButton);
        const cookieCheckboxBasic = element.$$('#cookiesCheckboxBasic');
        assertTrue(!!cookieCheckboxBasic);
        const basicTab = element.$$('#basicTabTitle');
        assertTrue(!!basicTab);
        const advancedTab = element.$$('#advancedTabTitle');
        assertTrue(!!advancedTab);
        // Initially the button is disabled because all checkboxes are off.
        assertTrue(actionButton.disabled);
        // The button gets enabled if any checkbox is selected.
        MockInteractions.tap(cookieCheckboxBasic.$.checkbox);
        assertTrue(cookieCheckboxBasic.checked);
        assertFalse(actionButton.disabled);
        // Switching to advanced disables the button.
        MockInteractions.tap(advancedTab);
        assertTrue(actionButton.disabled);
        // Switching back enables it again.
        MockInteractions.tap(basicTab);
        assertFalse(actionButton.disabled);
      });

      test('showHistoryDeletionDialog', function() {
        assertTrue(element.$$('#clearBrowsingDataDialog').open);
        const actionButton = element.$$('.action-button');
        assertTrue(!!actionButton);

        // Select a datatype for deletion to enable the clear button.
        const cookieCheckbox = element.$$('#cookiesCheckboxBasic');
        assertTrue(!!cookieCheckbox);
        MockInteractions.tap(cookieCheckbox.$.checkbox);
        assertFalse(actionButton.disabled);

        const promiseResolver = new PromiseResolver();
        testBrowserProxy.setClearBrowsingDataPromise(promiseResolver.promise);
        MockInteractions.tap(actionButton);

        return testBrowserProxy.whenCalled('clearBrowsingData')
            .then(function() {
              // Passing showNotice = true should trigger the notice about other
              // forms of browsing history to open, and the dialog to stay open.
              promiseResolver.resolve(true /* showNotice */);

              // Yields to the message loop to allow the callback chain of the
              // Promise that was just resolved to execute before the
              // assertions.
            })
            .then(function() {
              Polymer.dom.flush();
              const notice = element.$$('#notice');
              assertTrue(!!notice);
              const noticeActionButton = notice.$$('.action-button');
              assertTrue(!!noticeActionButton);

              assertTrue(element.$$('#clearBrowsingDataDialog').open);
              assertTrue(notice.$$('#dialog').open);

              MockInteractions.tap(noticeActionButton);

              return new Promise(function(resolve, reject) {
                // Tapping the action button will close the notice. Move to the
                // end of the message loop to allow the closing event to
                // propagate to the parent dialog. The parent dialog should
                // subsequently close as well.
                setTimeout(function() {
                  const notice = element.$$('#notice');
                  assertFalse(!!notice);
                  assertFalse(element.$$('#clearBrowsingDataDialog').open);
                  resolve();
                }, 0);
              });
            });
      });

      test('Counters', function() {
        assertTrue(element.$$('#clearBrowsingDataDialog').open);

        const checkbox = element.$$('#cacheCheckboxBasic');
        assertEquals('browser.clear_data.cache_basic', checkbox.pref.key);

        // Simulate a browsing data counter result for history. This checkbox's
        // sublabel should be updated.
        cr.webUIListenerCallback(
            'update-counter-text', checkbox.pref.key, 'result');
        assertEquals('result', checkbox.subLabel);
      });

      test('history rows are hidden for supervised users', function() {
        assertFalse(loadTimeData.getBoolean('isSupervised'));
        assertFalse(element.$$('#browsingCheckbox').hidden);
        assertFalse(element.$$('#browsingCheckboxBasic').hidden);
        assertFalse(element.$$('#downloadCheckbox').hidden);

        element.remove();
        testBrowserProxy.reset();
        loadTimeData.overrideValues({isSupervised: true});

        element = document.createElement('settings-clear-browsing-data-dialog');
        document.body.appendChild(element);
        Polymer.dom.flush();

        return testBrowserProxy.whenCalled('initialize').then(function() {
          assertTrue(element.$$('#browsingCheckbox').hidden);
          assertTrue(element.$$('#browsingCheckboxBasic').hidden);
          assertTrue(element.$$('#downloadCheckbox').hidden);
        });
      });
    });
  }

  function registerImportantSitesTests() {
    suite('ImportantSites', function() {
      /** @type {settings.TestClearBrowsingDataBrowserProxy} */
      let testBrowserProxy;

      /** @type {SettingsClearBrowsingDataDialogElement} */
      let element;

      /** @type {Array<ImportantSite>} */
      const importantSites = [
        {registerableDomain: 'google.com', isChecked: true},
        {registerableDomain: 'yahoo.com', isChecked: true}
      ];

      setup(function() {
        loadTimeData.overrideValues({importantSitesInCbd: true});
        testBrowserProxy = new TestClearBrowsingDataBrowserProxy();
        testBrowserProxy.setImportantSites(importantSites);
        settings.ClearBrowsingDataBrowserProxyImpl.instance_ = testBrowserProxy;
        PolymerTest.clearBody();
        element = document.createElement('settings-clear-browsing-data-dialog');
        element.set('prefs', getClearBrowsingDataPrefs());
        document.body.appendChild(element);
        return testBrowserProxy.whenCalled('initialize').then(function() {
          return testBrowserProxy.whenCalled('getImportantSites');
        });
      });

      teardown(function() {
        element.remove();
      });

      test('getImportantSites', function() {
        assertTrue(element.$$('#clearBrowsingDataDialog').open);
        assertFalse(element.showImportantSitesDialog_);
        // Select cookie checkbox.
        MockInteractions.tap(element.$$('#cookiesCheckboxBasic').$.checkbox);
        assertTrue(element.$$('#cookiesCheckboxBasic').checked);
        // Clear browsing data.
        MockInteractions.tap(element.$$('#clearBrowsingDataConfirm'));
        Polymer.dom.flush();
        assertFalse(element.$$('#clearBrowsingDataDialog').open);
        assertTrue(element.showImportantSitesDialog_);
        return new Promise(function(resolve) {
                 element.async(resolve);
               })
            .then(function() {
              assertTrue(element.$$('#importantSitesDialog').open);
              const firstImportantSite = element.$$('important-site-checkbox');
              assertTrue(!!firstImportantSite);
              assertEquals(
                  'google.com', firstImportantSite.site.registerableDomain);
              assertTrue(firstImportantSite.site.isChecked);
              // Choose to keep storage for google.com.
              MockInteractions.tap(firstImportantSite.$$('#checkbox'));
              assertFalse(firstImportantSite.site.isChecked);
              // Confirm deletion.
              MockInteractions.tap(element.$$('#importantSitesConfirm'));
              return testBrowserProxy.whenCalled('clearBrowsingData')
                  .then(function([dataTypes, timePeriod, sites]) {
                    assertEquals(1, dataTypes.length);
                    assertEquals(
                        'browser.clear_data.cookies_basic', dataTypes[0]);
                    assertEquals(2, sites.length);
                    assertEquals('google.com', sites[0].registerableDomain);
                    assertFalse(sites[0].isChecked);
                    assertEquals('yahoo.com', sites[1].registerableDomain);
                    assertTrue(sites[1].isChecked);
                  });
            });
      });
    });
  }

  if (cr.isMac || cr.isWin)
    registerNativeCertificateManagerTests();

  registerClearBrowsingDataTests();
  registerImportantSitesTests();
  registerPrivacyPageTests();
});
