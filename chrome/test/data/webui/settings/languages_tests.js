// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('settings-languages', function() {
  suite('settings-languages', function() {
    /**
     * @param {!Array<string>} expected
     */
    function assertLanguageOrder(expected) {
      assertEquals(expected.length, languageHelper.languages.enabled.length);
      for (let i = 0; i < expected.length; i++) {
        assertEquals(
            expected[i], languageHelper.languages.enabled[i].language.code);
      }
    }

    /** @type {?settings.LanguagesBrowserProxy} */
    let browserProxy = null;

    let languageHelper;

    suiteSetup(function() {
      CrSettingsPrefs.deferInitialization = true;
      PolymerTest.clearBody();
    });

    setup(function() {
      const settingsPrefs = document.createElement('settings-prefs');
      const settingsPrivate =
          new settings.FakeSettingsPrivate(settings.getFakeLanguagePrefs());
      settingsPrefs.initialize(settingsPrivate);
      document.body.appendChild(settingsPrefs);

      // Setup test browser proxy.
      browserProxy = new settings.TestLanguagesBrowserProxy();
      settings.LanguagesBrowserProxyImpl.instance_ = browserProxy;

      // Setup fake languageSettingsPrivate API.
      const languageSettingsPrivate = browserProxy.getLanguageSettingsPrivate();
      languageSettingsPrivate.setSettingsPrefs(settingsPrefs);

      languageHelper = document.createElement('settings-languages');

      // Prefs would normally be data-bound to settings-languages.
      test_util.fakeDataBind(settingsPrefs, languageHelper, 'prefs');

      document.body.appendChild(languageHelper);
      return languageHelper.whenReady().then(function() {
        if (cr.isChromeOS || cr.isWindows)
          return browserProxy.whenCalled('getProspectiveUILanguage');
      });
    });

    test('languages model', function() {
      const languageSettingsPrivate = browserProxy.getLanguageSettingsPrivate();
      for (let i = 0; i < languageSettingsPrivate.languages.length; i++) {
        assertEquals(
            languageSettingsPrivate.languages[i].code,
            languageHelper.languages.supported[i].code);
      }
      assertLanguageOrder(['en-US', 'sw']);
      assertEquals('en', languageHelper.languages.translateTarget);

      // TODO(michaelpg): Test other aspects of the model.
    });

    test('modifying languages', function() {
      assertTrue(languageHelper.isLanguageEnabled('en-US'));
      assertTrue(languageHelper.isLanguageEnabled('sw'));
      assertFalse(languageHelper.isLanguageEnabled('en-CA'));

      languageHelper.enableLanguage('en-CA');
      assertTrue(languageHelper.isLanguageEnabled('en-CA'));
      languageHelper.disableLanguage('sw');
      assertFalse(languageHelper.isLanguageEnabled('sw'));

      // TODO(michaelpg): Test other modifications.
    });

    test('reorder languages', function() {
      // New language is added at the end.
      languageHelper.enableLanguage('en-CA');
      assertLanguageOrder(['en-US', 'sw', 'en-CA']);

      // Can move a language up.
      languageHelper.moveLanguage('en-CA', true /* upDirection */);
      assertLanguageOrder(['en-US', 'en-CA', 'sw']);

      // Can move a language down.
      languageHelper.moveLanguage('en-US', false /* upDirection */);
      assertLanguageOrder(['en-CA', 'en-US', 'sw']);

      // Can move a language to the front.
      languageHelper.moveLanguageToFront('sw');
      const expectedOrder = ['sw', 'en-CA', 'en-US'];
      assertLanguageOrder(expectedOrder);

      // Moving the first language up has no effect.
      languageHelper.moveLanguage('sw', true /* upDirection */);
      assertLanguageOrder(expectedOrder);

      // Moving the first language to top has no effect.
      languageHelper.moveLanguageToFront('sw');
      assertLanguageOrder(expectedOrder);

      // Moving the last language down has no effect.
      languageHelper.moveLanguage('en-US', false /* upDirection */);
      assertLanguageOrder(expectedOrder);
    });

    if (cr.isChromeOS) {
      test('modifying input methods', function() {
        assertEquals(2, languageHelper.languages.inputMethods.enabled.length);
        const inputMethods = languageHelper.getInputMethodsForLanguage('en-US');
        assertEquals(3, inputMethods.length);

        // We can remove one input method.
        const dvorak =
            '_comp_ime_fgoepimhcoialccpbmpnnblemnepkkaoxkb:us:dvorak:eng';
        languageHelper.removeInputMethod(dvorak);
        assertEquals(1, languageHelper.languages.inputMethods.enabled.length);

        // Enable Swahili.
        languageHelper.enableLanguage('sw');
        assertEquals(1, languageHelper.languages.inputMethods.enabled.length);

        // Add input methods for Swahili.
        const sw = '_comp_ime_abcdefghijklmnopqrstuvwxyzabcdefxkb:sw:sw';
        const swUS = '_comp_ime_abcdefghijklmnopqrstuvwxyzabcdefxkb:us:sw';
        languageHelper.addInputMethod(sw);
        languageHelper.addInputMethod(swUS);
        assertEquals(3, languageHelper.languages.inputMethods.enabled.length);

        // Disable Swahili. The Swahili-only keyboard should be removed.
        languageHelper.disableLanguage('sw');
        assertEquals(2, languageHelper.languages.inputMethods.enabled.length);

        // The US Swahili keyboard should still be enabled, because it supports
        // English which is still enabled.
        assertTrue(languageHelper.languages.inputMethods.enabled.some(function(
            inputMethod) {
          return inputMethod.id == swUS;
        }));
      });
    }
  });
});
