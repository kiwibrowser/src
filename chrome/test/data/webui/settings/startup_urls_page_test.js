// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('settings_startup_urls_page', function() {
  /** @implements {settings.StartupUrlsPageBrowserProxy} */
  class TestStartupUrlsPageBrowserProxy extends TestBrowserProxy {
    constructor() {
      super([
        'addStartupPage',
        'editStartupPage',
        'loadStartupPages',
        'removeStartupPage',
        'useCurrentPages',
        'validateStartupPage',
      ]);

      /** @private {boolean} */
      this.urlIsValid_ = true;
    }

    /** @param {boolean} isValid */
    setUrlValidity(isValid) {
      this.urlIsValid_ = isValid;
    }

    /** @override */
    addStartupPage(url) {
      this.methodCalled('addStartupPage', url);
      return Promise.resolve(this.urlIsValid_);
    }

    /** @override */
    editStartupPage(modelIndex, url) {
      this.methodCalled('editStartupPage', [modelIndex, url]);
      return Promise.resolve(this.urlIsValid_);
    }

    /** @override */
    loadStartupPages() {
      this.methodCalled('loadStartupPages');
    }

    /** @override */
    removeStartupPage(modelIndex) {
      this.methodCalled('removeStartupPage', modelIndex);
    }

    /** @override */
    useCurrentPages() {
      this.methodCalled('useCurrentPages');
    }

    /** @override */
    validateStartupPage(url) {
      this.methodCalled('validateStartupPage', url);
      return Promise.resolve(this.urlIsValid_);
    }
  }

  suite('StartupUrlDialog', function() {
    /** @type {?SettingsStartupUrlDialogElement} */
    let dialog = null;

    let browserProxy = null;

    /**
     * Triggers an 'input' event on the given text input field, which triggers
     * validation to occur.
     * @param {!PaperInputElement} element
     */
    function pressSpace(element) {
      // The actual key code is irrelevant for these tests.
      MockInteractions.keyEventOn(element, 'input', 32 /* space key code */);
    }

    setup(function() {
      browserProxy = new TestStartupUrlsPageBrowserProxy();
      settings.StartupUrlsPageBrowserProxyImpl.instance_ = browserProxy;
      PolymerTest.clearBody();
      dialog = document.createElement('settings-startup-url-dialog');
    });

    teardown(function() {
      dialog.remove();
    });

    test('Initialization_Add', function() {
      document.body.appendChild(dialog);
      Polymer.dom.flush();
      assertTrue(dialog.$.dialog.open);

      // Assert that the "Add" button is disabled.
      const actionButton = dialog.$.actionButton;
      assertTrue(!!actionButton);
      assertTrue(actionButton.disabled);

      // Assert that the text field is empty.
      const inputElement = dialog.$.url;
      assertTrue(!!inputElement);
      assertEquals('', inputElement.value);
    });

    test('Initialization_Edit', function() {
      dialog.model = createSampleUrlEntry();
      document.body.appendChild(dialog);
      assertTrue(dialog.$.dialog.open);

      // Assert that the "Edit" button is enabled.
      const actionButton = dialog.$.actionButton;
      assertTrue(!!actionButton);
      assertFalse(actionButton.disabled);
      // Assert that the text field is pre-populated.
      const inputElement = dialog.$.url;
      assertTrue(!!inputElement);
      assertEquals(dialog.model.url, inputElement.value);
    });

    // Test that validation occurs as the user is typing, and that the action
    // button is updated accordingly.
    test('Validation', function() {
      document.body.appendChild(dialog);

      const actionButton = dialog.$.actionButton;
      assertTrue(actionButton.disabled);
      const inputElement = dialog.$.url;

      const expectedUrl = 'dummy-foo.com';
      inputElement.value = expectedUrl;
      browserProxy.setUrlValidity(false);
      pressSpace(inputElement);

      return browserProxy.whenCalled('validateStartupPage')
          .then(function(url) {
            assertEquals(expectedUrl, url);
            assertTrue(actionButton.disabled);
            assertTrue(!!inputElement.invalid);

            browserProxy.setUrlValidity(true);
            browserProxy.resetResolver('validateStartupPage');
            pressSpace(inputElement);

            return browserProxy.whenCalled('validateStartupPage');
          })
          .then(function() {
            assertFalse(actionButton.disabled);
            assertFalse(!!inputElement.invalid);
          });
    });

    /**
     * Tests that the appropriate browser proxy method is called when the action
     * button is tapped.
     * @param {string} proxyMethodName
     */
    function testProxyCalled(proxyMethodName) {
      const actionButton = dialog.$.actionButton;
      actionButton.disabled = false;

      // Test that the dialog remains open if the user somehow manages to submit
      // an invalid URL.
      browserProxy.setUrlValidity(false);
      MockInteractions.tap(actionButton);
      return browserProxy.whenCalled(proxyMethodName)
          .then(function() {
            assertTrue(dialog.$.dialog.open);

            // Test that dialog is closed if the user submits a valid URL.
            browserProxy.setUrlValidity(true);
            browserProxy.resetResolver(proxyMethodName);
            MockInteractions.tap(actionButton);
            return browserProxy.whenCalled(proxyMethodName);
          })
          .then(function() {
            assertFalse(dialog.$.dialog.open);
          });
    }

    test('AddStartupPage', function() {
      document.body.appendChild(dialog);
      return testProxyCalled('addStartupPage');
    });

    test('EditStartupPage', function() {
      dialog.model = createSampleUrlEntry();
      document.body.appendChild(dialog);
      return testProxyCalled('editStartupPage');
    });

    test('Enter key submits', function() {
      document.body.appendChild(dialog);

      // Input a URL and force validation.
      const inputElement = dialog.$.url;
      inputElement.value = 'foo.com';
      pressSpace(inputElement);

      return browserProxy.whenCalled('validateStartupPage').then(function() {
        MockInteractions.keyEventOn(
            inputElement, 'keypress', 13, undefined, 'Enter');

        return browserProxy.whenCalled('addStartupPage');
      });
    });
  });

  suite('StartupUrlsPage', function() {
    /** @type {?SettingsStartupUrlsPageElement} */
    let page = null;

    let browserProxy = null;

    setup(function() {
      browserProxy = new TestStartupUrlsPageBrowserProxy();
      settings.StartupUrlsPageBrowserProxyImpl.instance_ = browserProxy;
      PolymerTest.clearBody();
      page = document.createElement('settings-startup-urls-page');
      page.prefs = {
        session: {
          restore_on_startup: {
            type: chrome.settingsPrivate.PrefType.NUMBER,
            value: 5,
          },
        },
      };
      document.body.appendChild(page);
      Polymer.dom.flush();
    });

    teardown(function() {
      page.remove();
    });

    // Test that the page is requesting information from the browser.
    test('Initialization', function() {
      return browserProxy.whenCalled('loadStartupPages');
    });

    test('UseCurrentPages', function() {
      const useCurrentPagesButton = page.$$('#useCurrentPages > a');
      assertTrue(!!useCurrentPagesButton);
      MockInteractions.tap(useCurrentPagesButton);
      return browserProxy.whenCalled('useCurrentPages');
    });

    test('AddPage_OpensDialog', function() {
      const addPageButton = page.$$('#addPage > a');
      assertTrue(!!addPageButton);
      assertFalse(!!page.$$('settings-startup-url-dialog'));

      MockInteractions.tap(addPageButton);
      Polymer.dom.flush();
      assertTrue(!!page.$$('settings-startup-url-dialog'));
    });

    test('EditPage_OpensDialog', function() {
      assertFalse(!!page.$$('settings-startup-url-dialog'));
      page.fire(
          settings.EDIT_STARTUP_URL_EVENT,
          {model: createSampleUrlEntry(), anchor: null});
      Polymer.dom.flush();
      assertTrue(!!page.$$('settings-startup-url-dialog'));
    });

    test('StartupPagesChanges_CloseOpenEditDialog', function() {
      const entry1 = {
        modelIndex: 2,
        title: 'Test page 1',
        tooltip: 'test tooltip',
        url: 'chrome://bar',
      };

      const entry2 = {
        modelIndex: 2,
        title: 'Test page 2',
        tooltip: 'test tooltip',
        url: 'chrome://foo',
      };

      cr.webUIListenerCallback('update-startup-pages', [entry1, entry2]);
      page.fire(settings.EDIT_STARTUP_URL_EVENT, {model: entry2, anchor: null});
      Polymer.dom.flush();

      assertTrue(!!page.$$('settings-startup-url-dialog'));
      cr.webUIListenerCallback('update-startup-pages', [entry1]);
      Polymer.dom.flush();

      assertFalse(!!page.$$('settings-startup-url-dialog'));
    });

    test('StartupPages_WhenExtensionControlled', function() {
      assertFalse(!!page.get('prefs.session.startup_urls.controlledBy'));
      assertFalse(!!page.$$('extension-controlled-indicator'));
      assertTrue(!!page.$$('#addPage'));
      assertTrue(!!page.$$('#useCurrentPages'));

      page.set('prefs.session.startup_urls', {
        controlledBy: chrome.settingsPrivate.ControlledBy.EXTENSION,
        controlledByName: 'Totally Real Extension',
        enforcement: chrome.settingsPrivate.Enforcement.ENFORCED,
        extensionId: 'mefmhpjnkplhdhmfmblilkgpkbjebmij',
        type: chrome.settingsPrivate.PrefType.NUMBER,
        value: 5,
      });
      Polymer.dom.flush();

      assertTrue(!!page.$$('extension-controlled-indicator'));
      assertFalse(!!page.$$('#addPage'));
      assertFalse(!!page.$$('#useCurrentPages'));
    });
  });

  /** @return {!StartupPageInfo} */
  function createSampleUrlEntry() {
    return {
      modelIndex: 2,
      title: 'Test page',
      tooltip: 'test tooltip',
      url: 'chrome://foo',
    };
  }

  suite('StartupUrlEntry', function() {
    /** @type {?SettingsStartupUrlEntryElement} */
    let element = null;

    let browserProxy = null;

    setup(function() {
      browserProxy = new TestStartupUrlsPageBrowserProxy();
      settings.StartupUrlsPageBrowserProxyImpl.instance_ = browserProxy;
      PolymerTest.clearBody();
      element = document.createElement('settings-startup-url-entry');
      element.model = createSampleUrlEntry();
      document.body.appendChild(element);
      Polymer.dom.flush();
    });

    teardown(function() {
      element.remove();
    });

    test('MenuOptions_Remove', function() {
      element.editable = true;
      Polymer.dom.flush();

      // Bring up the popup menu.
      assertFalse(!!element.$$('cr-action-menu'));
      MockInteractions.tap(element.$$('#dots'));
      Polymer.dom.flush();
      assertTrue(!!element.$$('cr-action-menu'));

      const removeButton = element.shadowRoot.querySelector('#remove');
      MockInteractions.tap(removeButton);
      return browserProxy.whenCalled('removeStartupPage')
          .then(function(modelIndex) {
            assertEquals(element.model.modelIndex, modelIndex);
          });
    });

    test('Editable', function() {
      assertFalse(!!element.editable);
      assertFalse(!!element.$$('#dots'));

      element.editable = true;
      Polymer.dom.flush();
      assertTrue(!!element.$$('#dots'));
    });
  });
});
