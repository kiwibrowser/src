// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('settings_people_page_sync_page', function() {

  suite('AdvancedSyncSettingsTests', function() {
    let syncPage = null;
    let browserProxy = null;
    let encryptWithGoogle = null;
    const encyyptWithPassphrase = null;

    /**
     * Returns sync prefs with everything synced and no passphrase required.
     * @return {!settings.SyncPrefs}
     */
    function getSyncAllPrefs() {
      return {
        appsEnforced: false,
        appsRegistered: true,
        appsSynced: true,
        autofillEnforced: false,
        autofillRegistered: true,
        autofillSynced: true,
        bookmarksEnforced: false,
        bookmarksRegistered: true,
        bookmarksSynced: true,
        encryptAllData: false,
        encryptAllDataAllowed: true,
        enterGooglePassphraseBody: 'Enter Google passphrase.',
        enterPassphraseBody: 'Enter custom passphrase.',
        extensionsEnforced: false,
        extensionsRegistered: true,
        extensionsSynced: true,
        fullEncryptionBody: '',
        passphrase: '',
        passphraseRequired: false,
        passphraseTypeIsCustom: false,
        passwordsEnforced: false,
        passwordsRegistered: true,
        passwordsSynced: true,
        paymentsIntegrationEnabled: true,
        preferencesEnforced: false,
        preferencesRegistered: true,
        preferencesSynced: true,
        setNewPassphrase: false,
        syncAllDataTypes: true,
        tabsEnforced: false,
        tabsRegistered: true,
        tabsSynced: true,
        themesEnforced: false,
        themesRegistered: true,
        themesSynced: true,
        typedUrlsEnforced: false,
        typedUrlsRegistered: true,
        typedUrlsSynced: true,
      };
    }

    setup(function() {
      browserProxy = new TestSyncBrowserProxy();
      settings.SyncBrowserProxyImpl.instance_ = browserProxy;

      PolymerTest.clearBody();
      syncPage = document.createElement('settings-sync-page');
      settings.navigateTo(settings.routes.SYNC);

      document.body.appendChild(syncPage);

      cr.webUIListenerCallback(
          'page-status-changed', settings.PageStatus.CONFIGURE);
      assertFalse(syncPage.$$('#' + settings.PageStatus.CONFIGURE).hidden);
      assertTrue(syncPage.$$('#' + settings.PageStatus.TIMEOUT).hidden);
      assertTrue(syncPage.$$('#' + settings.PageStatus.SPINNER).hidden);

      // Start with Sync All with no encryption selected.
      cr.webUIListenerCallback('sync-prefs-changed', getSyncAllPrefs());
      Polymer.dom.flush();

      encryptWithGoogle =
          syncPage.$$('cr-radio-button[name="encrypt-with-google"]');
      encryptWithPassphrase =
          syncPage.$$('cr-radio-button[name="encrypt-with-passphrase"]');
      assertTrue(!!encryptWithGoogle);
      assertTrue(!!encryptWithPassphrase);
    });

    teardown(function() {
      syncPage.remove();
    });

    test('NotifiesHandlerOfNavigation', function() {
      function testNavigateAway() {
        settings.navigateTo(settings.routes.PEOPLE);
        return browserProxy.whenCalled('didNavigateAwayFromSyncPage');
      }

      function testNavigateBack() {
        browserProxy.resetResolver('didNavigateToSyncPage');
        settings.navigateTo(settings.routes.SYNC);
        return browserProxy.whenCalled('didNavigateToSyncPage');
      }

      function testDetach() {
        browserProxy.resetResolver('didNavigateAwayFromSyncPage');
        syncPage.remove();
        return browserProxy.whenCalled('didNavigateAwayFromSyncPage');
      }

      function testRecreate() {
        browserProxy.resetResolver('didNavigateToSyncPage');
        syncPage = document.createElement('settings-sync-page');
        settings.navigateTo(settings.routes.SYNC);

        document.body.appendChild(syncPage);
        return browserProxy.whenCalled('didNavigateToSyncPage');
      }

      return browserProxy.whenCalled('didNavigateToSyncPage')
          .then(testNavigateAway)
          .then(testNavigateBack)
          .then(testDetach)
          .then(testRecreate);
    });

    test('SyncSectionLayout_NoUnifiedConsent_SignedIn', function() {
      const ironCollapse = syncPage.$$('#sync-section');
      const otherItems = syncPage.$$('#other-sync-items');
      const syncSectionToggle = syncPage.$$('#sync-section-toggle');

      // When unified-consent is disabled and signed in, sync-section should be
      // visible and open by default. Accordion toggle row should not be present
      // and bottom items should not have classes used for indentation.
      syncPage.syncStatus = {signedIn: true};
      syncPage.unifiedConsentEnabled = false;
      Polymer.dom.flush();
      assertTrue(ironCollapse.opened);
      assertFalse(ironCollapse.hidden);
      assertTrue(syncSectionToggle.hidden);
      assertFalse(otherItems.classList.contains('list-frame'));
      assertFalse(!!otherItems.querySelector('list-item'));
    });

    test('SyncSectionLayout_UnifiedConsentEnabled_SignedIn', function() {
      const ironCollapse = syncPage.$$('#sync-section');
      const otherItems = syncPage.$$('#other-sync-items');
      const syncSectionToggle = syncPage.$$('#sync-section-toggle');
      const expandIcon = syncSectionToggle.querySelector('cr-expand-button');

      // When unified-consent is enabled and signed in, sync-section should be
      // visible and open by default. Accordion toggle row should be present,
      // and bottom items should have classes used for indentation.
      syncPage.syncStatus = {signedIn: true};
      syncPage.unifiedConsentEnabled = true;
      Polymer.dom.flush();
      assertTrue(ironCollapse.opened);
      assertFalse(ironCollapse.hidden);
      assertFalse(syncSectionToggle.hidden);
      assertTrue(syncSectionToggle.hasAttribute('actionable'));
      assertTrue(expandIcon.expanded);
      assertFalse(expandIcon.disabled);
      assertTrue(otherItems.classList.contains('list-frame'));
      assertEquals(
          otherItems.querySelectorAll(':scope > .list-item').length, 3);

      // Tapping on the toggle row should toggle ironCollapse.
      MockInteractions.tap(syncSectionToggle);
      Polymer.dom.flush();
      assertFalse(ironCollapse.opened);
      assertFalse(expandIcon.expanded);
      MockInteractions.tap(syncSectionToggle);
      Polymer.dom.flush();
      assertTrue(ironCollapse.opened);
      assertTrue(expandIcon.expanded);
    });

    test('SyncSectionLayout_UnifiedConsentEnabled_SignedOut', function() {
      const ironCollapse = syncPage.$$('#sync-section');
      const syncSectionToggle = syncPage.$$('#sync-section-toggle');
      const expandIcon = syncSectionToggle.querySelector('cr-expand-button');

      // When unified-consent is enabled and signed out, sync-section should be
      // hidden, and the accordion toggle row should be visible not actionable.
      syncPage.syncStatus = {signedIn: false};
      syncPage.unifiedConsentEnabled = true;
      Polymer.dom.flush();
      assertTrue(ironCollapse.hidden);
      assertFalse(syncSectionToggle.hidden);
      assertFalse(syncSectionToggle.hasAttribute('actionable'));
      assertFalse(expandIcon.expanded);
      assertTrue(expandIcon.disabled);
    });

    test('LoadingAndTimeout', function() {
      const configurePage = syncPage.$$('#' + settings.PageStatus.CONFIGURE);
      const spinnerPage = syncPage.$$('#' + settings.PageStatus.SPINNER);
      const timeoutPage = syncPage.$$('#' + settings.PageStatus.TIMEOUT);

      cr.webUIListenerCallback(
          'page-status-changed', settings.PageStatus.SPINNER);
      assertTrue(configurePage.hidden);
      assertTrue(timeoutPage.hidden);
      assertFalse(spinnerPage.hidden);

      cr.webUIListenerCallback(
          'page-status-changed', settings.PageStatus.TIMEOUT);
      assertTrue(configurePage.hidden);
      assertFalse(timeoutPage.hidden);
      assertTrue(spinnerPage.hidden);

      cr.webUIListenerCallback(
          'page-status-changed', settings.PageStatus.CONFIGURE);
      assertFalse(configurePage.hidden);
      assertTrue(timeoutPage.hidden);
      assertTrue(spinnerPage.hidden);

      // Should remain on the CONFIGURE page even if the passphrase failed.
      cr.webUIListenerCallback(
          'page-status-changed', settings.PageStatus.PASSPHRASE_FAILED);
      assertFalse(configurePage.hidden);
      assertTrue(timeoutPage.hidden);
      assertTrue(spinnerPage.hidden);
    });

    test('SettingIndividualDatatypes', function() {
      const syncAllDataTypesControl = syncPage.$.syncAllDataTypesControl;
      assertFalse(syncAllDataTypesControl.disabled);
      assertTrue(syncAllDataTypesControl.checked);

      // Assert that all the individual datatype controls are disabled.
      const datatypeControls =
          syncPage.$$('#configure').querySelectorAll('.list-item cr-toggle');
      for (const control of datatypeControls) {
        assertTrue(control.disabled);
        assertTrue(control.checked);
      }

      // Uncheck the Sync All control.
      MockInteractions.tap(syncAllDataTypesControl);

      function verifyPrefs(prefs) {
        const expected = getSyncAllPrefs();
        expected.syncAllDataTypes = false;
        assertEquals(JSON.stringify(expected), JSON.stringify(prefs));

        cr.webUIListenerCallback('sync-prefs-changed', expected);

        // Assert that all the individual datatype controls are enabled.
        for (const control of datatypeControls) {
          assertFalse(control.disabled);
          assertTrue(control.checked);
        }

        browserProxy.resetResolver('setSyncDatatypes');

        // Test an arbitrarily-selected control (extensions synced control).
        MockInteractions.tap(datatypeControls[3]);
        return browserProxy.whenCalled('setSyncDatatypes')
            .then(function(prefs) {
              const expected = getSyncAllPrefs();
              expected.syncAllDataTypes = false;
              expected.extensionsSynced = false;
              assertEquals(JSON.stringify(expected), JSON.stringify(prefs));
            });
      }
      return browserProxy.whenCalled('setSyncDatatypes').then(verifyPrefs);
    });

    test('RadioBoxesEnabledWhenUnencrypted', function() {
      // Verify that the encryption radio boxes are enabled.
      assertFalse(encryptWithGoogle.disabled);
      assertFalse(encryptWithPassphrase.disabled);

      assertTrue(encryptWithGoogle.checked);

      // Select 'Encrypt with passphrase' to create a new passphrase.
      assertFalse(!!syncPage.$$('#create-password-box'));

      MockInteractions.tap(encryptWithPassphrase);
      Polymer.dom.flush();

      assertTrue(!!syncPage.$$('#create-password-box'));
      const saveNewPassphrase = syncPage.$$('#saveNewPassphrase');
      assertTrue(!!saveNewPassphrase);

      // Test that a sync prefs update does not reset the selection.
      cr.webUIListenerCallback('sync-prefs-changed', getSyncAllPrefs());
      Polymer.dom.flush();
      assertTrue(encryptWithPassphrase.checked);
    });

    test('ClickingLinkDoesNotChangeRadioValue', function() {
      assertFalse(encryptWithPassphrase.disabled);
      assertFalse(encryptWithPassphrase.checked);

      const link = encryptWithPassphrase.querySelector('a[href]');
      assertTrue(!!link);

      // Suppress opening a new tab, since then the test will continue running
      // on a background tab (which has throttled timers) and will timeout.
      link.target = '';
      link.href = '#';
      // Prevent the link from triggering a page navigation when tapped.
      // Breaks the test in Vulcanized mode.
      link.addEventListener('click', function(e) {
        e.preventDefault();
      });

      MockInteractions.tap(link);

      assertFalse(encryptWithPassphrase.checked);
    });

    test('SaveButtonDisabledWhenPassphraseOrConfirmationEmpty', function() {
      MockInteractions.tap(encryptWithPassphrase);
      Polymer.dom.flush();

      assertTrue(!!syncPage.$$('#create-password-box'));
      const saveNewPassphrase = syncPage.$$('#saveNewPassphrase');
      const passphraseInput = syncPage.$$('#passphraseInput');
      const passphraseConfirmationInput =
          syncPage.$$('#passphraseConfirmationInput');

      passphraseInput.value = '';
      passphraseConfirmationInput.value = '';
      assertTrue(saveNewPassphrase.disabled);

      passphraseInput.value = 'foo';
      passphraseConfirmationInput.value = '';
      assertTrue(saveNewPassphrase.disabled);

      passphraseInput.value = 'foo';
      passphraseConfirmationInput.value = 'bar';
      assertFalse(saveNewPassphrase.disabled);
    });

    test('CreatingPassphraseMismatchedPassphrase', function() {
      MockInteractions.tap(encryptWithPassphrase);
      Polymer.dom.flush();

      assertTrue(!!syncPage.$$('#create-password-box'));
      const saveNewPassphrase = syncPage.$$('#saveNewPassphrase');
      assertTrue(!!saveNewPassphrase);

      const passphraseInput = syncPage.$$('#passphraseInput');
      const passphraseConfirmationInput =
          syncPage.$$('#passphraseConfirmationInput');
      passphraseInput.value = 'foo';
      passphraseConfirmationInput.value = 'bar';

      MockInteractions.tap(saveNewPassphrase);
      Polymer.dom.flush();

      assertFalse(passphraseInput.invalid);
      assertTrue(passphraseConfirmationInput.invalid);

      assertFalse(syncPage.syncPrefs.encryptAllData);
    });

    test('CreatingPassphraseValidPassphrase', function() {
      MockInteractions.tap(encryptWithPassphrase);
      Polymer.dom.flush();

      assertTrue(!!syncPage.$$('#create-password-box'));
      const saveNewPassphrase = syncPage.$$('#saveNewPassphrase');
      assertTrue(!!saveNewPassphrase);

      const passphraseInput = syncPage.$$('#passphraseInput');
      const passphraseConfirmationInput =
          syncPage.$$('#passphraseConfirmationInput');
      passphraseInput.value = 'foo';
      passphraseConfirmationInput.value = 'foo';
      MockInteractions.tap(saveNewPassphrase);

      function verifyPrefs(prefs) {
        const expected = getSyncAllPrefs();
        expected.setNewPassphrase = true;
        expected.passphrase = 'foo';
        expected.encryptAllData = true;
        assertEquals(JSON.stringify(expected), JSON.stringify(prefs));

        expected.fullEncryptionBody = 'Encrypted with custom passphrase';
        cr.webUIListenerCallback('sync-prefs-changed', expected);

        Polymer.dom.flush();

        // Assert that the radio boxes are disabled after encryption enabled.
        assertTrue(encryptWithGoogle.disabled);
        assertTrue(encryptWithPassphrase.disabled);
      }
      return browserProxy.whenCalled('setSyncEncryption').then(verifyPrefs);
    });

    test('RadioBoxesHiddenWhenEncrypted', function() {
      const prefs = getSyncAllPrefs();
      prefs.encryptAllData = true;
      prefs.passphraseRequired = true;
      prefs.fullEncryptionBody = 'Sync already encrypted.';
      cr.webUIListenerCallback('sync-prefs-changed', prefs);

      Polymer.dom.flush();

      assertTrue(syncPage.$.encryptionDescription.hidden);
      assertTrue(syncPage.$.encryptionRadioGroupContainer.hidden);
    });

    test(
        'ExistingPassphraseSubmitButtonDisabledWhenExistingPassphraseEmpty',
        function() {
          const prefs = getSyncAllPrefs();
          prefs.encryptAllData = true;
          prefs.passphraseRequired = true;
          cr.webUIListenerCallback('sync-prefs-changed', prefs);

          Polymer.dom.flush();

          const existingPassphraseInput =
              syncPage.$$('#existingPassphraseInput');
          const submitExistingPassphrase =
              syncPage.$$('#submitExistingPassphrase');

          existingPassphraseInput.value = '';
          assertTrue(submitExistingPassphrase.disabled);

          existingPassphraseInput.value = 'foo';
          assertFalse(submitExistingPassphrase.disabled);
        });

    test('EnterExistingWrongPassphrase', function() {
      const prefs = getSyncAllPrefs();
      prefs.encryptAllData = true;
      prefs.passphraseRequired = true;
      cr.webUIListenerCallback('sync-prefs-changed', prefs);

      Polymer.dom.flush();

      const existingPassphraseInput = syncPage.$$('#existingPassphraseInput');
      assertTrue(!!existingPassphraseInput);
      existingPassphraseInput.value = 'wrong';
      browserProxy.encryptionResponse = settings.PageStatus.PASSPHRASE_FAILED;

      const submitExistingPassphrase = syncPage.$$('#submitExistingPassphrase');
      assertTrue(!!submitExistingPassphrase);
      MockInteractions.tap(submitExistingPassphrase);

      return browserProxy.whenCalled('setSyncEncryption').then(function(prefs) {
        const expected = getSyncAllPrefs();
        expected.setNewPassphrase = false;
        expected.passphrase = 'wrong';
        expected.encryptAllData = true;
        expected.passphraseRequired = true;
        assertEquals(JSON.stringify(expected), JSON.stringify(prefs));

        Polymer.dom.flush();

        assertTrue(existingPassphraseInput.invalid);
      });
    });

    test('EnterExistingCorrectPassphrase', function() {
      const prefs = getSyncAllPrefs();
      prefs.encryptAllData = true;
      prefs.passphraseRequired = true;
      cr.webUIListenerCallback('sync-prefs-changed', prefs);

      Polymer.dom.flush();

      const existingPassphraseInput = syncPage.$$('#existingPassphraseInput');
      assertTrue(!!existingPassphraseInput);
      existingPassphraseInput.value = 'right';
      browserProxy.encryptionResponse = settings.PageStatus.CONFIGURE;

      const submitExistingPassphrase = syncPage.$$('#submitExistingPassphrase');
      assertTrue(!!submitExistingPassphrase);
      MockInteractions.tap(submitExistingPassphrase);

      return browserProxy.whenCalled('setSyncEncryption').then(function(prefs) {
        const expected = getSyncAllPrefs();
        expected.setNewPassphrase = false;
        expected.passphrase = 'right';
        expected.encryptAllData = true;
        expected.passphraseRequired = true;
        assertEquals(JSON.stringify(expected), JSON.stringify(prefs));

        const newPrefs = getSyncAllPrefs();
        newPrefs.encryptAllData = true;
        cr.webUIListenerCallback('sync-prefs-changed', newPrefs);

        Polymer.dom.flush();

        // Verify that the encryption radio boxes are shown but disabled.
        assertTrue(encryptWithGoogle.disabled);
        assertTrue(encryptWithPassphrase.disabled);
      });
    });

    if (!cr.isChromeOS) {
      test('FirstTimeSetupNotification', function() {
        assertTrue(!!syncPage.$.toast);
        assertFalse(syncPage.$.toast.open);
        syncPage.syncStatus = {setupInProgress: true};
        Polymer.dom.flush();
        assertTrue(syncPage.$.toast.open);

        MockInteractions.tap(syncPage.$.toast.querySelector('paper-button'));

        return browserProxy.whenCalled('didNavigateAwayFromSyncPage')
            .then(abort => {
              assertTrue(abort);
            });
      });

      test('ShowAccountRow', function() {
        assertFalse(!!syncPage.$$('settings-sync-account-control'));
        syncPage.diceEnabled = true;
        Polymer.dom.flush();
        assertFalse(!!syncPage.$$('settings-sync-account-control'));
        syncPage.unifiedConsentEnabled = true;
        syncPage.syncStatus = {signinAllowed: false, syncSystemEnabled: false};
        Polymer.dom.flush();
        assertFalse(!!syncPage.$$('settings-sync-account-control'));
        syncPage.syncStatus = {signinAllowed: true, syncSystemEnabled: true};
        Polymer.dom.flush();
        assertTrue(!!syncPage.$$('settings-sync-account-control'));
      });
    }
  });
});
