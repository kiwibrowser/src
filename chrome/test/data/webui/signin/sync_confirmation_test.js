// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('signin_sync_confirmation', function() {

  suite('SigninSyncConfirmationTest', function() {
    let app;
    setup(function() {
      PolymerTest.clearBody();
      app = document.createElement('sync-confirmation-app');
      document.body.append(app);
    });

    // Tests that no DCHECKS are thrown during initialization of the UI.
    test('LoadPage', function() {
      assertEquals(
          'Get Google smarts in Chrome',
          app.$.syncConfirmationHeading.textContent.trim());
    });
  });

  // This test suite verifies that the consent strings recorded in various
  // scenarios are as expected. If the corresponding HTML file was updated
  // without also updating the attributes referring to consent strings,
  // this test will break.
  suite('SigninSyncConfirmationConsentRecordingTest', function() {
    let app;
    let browserProxy;

    setup(function() {
      // This test suite makes comparisons with strings in their default locale,
      // which is en-US.
      assertEquals(
          'en-US', navigator.language,
          'Cannot verify strings for the ' + navigator.language + 'locale.');

      browserProxy = new TestSyncConfirmationBrowserProxy();
      sync.confirmation.SyncConfirmationBrowserProxyImpl.instance_ =
          browserProxy;

      PolymerTest.clearBody();
      app = document.createElement('sync-confirmation-app');
      document.body.append(app);
    });

    const STANDARD_CONSENT_DESCRIPTION_TEXT = [
      'Get Google smarts in Chrome',
      'Your bookmarks, passwords, history, and more on all your devices',
      'Personalized Google services like Google Pay',
      'Improve Chrome and its security by sending system and usage ' +
          'information to Google',
      'Google may use content on sites you visit and browsing activity and ' +
          'interactions to personalize Chrome and other Google services like ' +
          'Translate, Search, and ads. You can customize this in Settings.',
    ];


    // Tests that the expected strings are recorded when clicking the Confirm
    // button.
    test('recordConsentOnConfirm', function() {
      app.$$('#confirmButton').click();
      return browserProxy.whenCalled('confirm').then(function(arguments) {
        assertEquals(2, arguments.length);
        var description = arguments[0];
        var confirmation = arguments[1];

        assertEquals(
            JSON.stringify(STANDARD_CONSENT_DESCRIPTION_TEXT),
            JSON.stringify(description));
        assertEquals('Yes, I\'m in', confirmation);
      });
    });

    // Tests that the expected strings are recorded when clicking the Confirm
    // button.
    test('recordConsentOnSettingsLink', function() {
      app.$$('#settingsButton').click();
      return browserProxy.whenCalled('goToSettings').then(function(arguments) {
        assertEquals(2, arguments.length);
        var description = arguments[0];
        var confirmation = arguments[1];

        assertEquals(
            JSON.stringify(STANDARD_CONSENT_DESCRIPTION_TEXT),
            JSON.stringify(description));
        assertEquals('Settings', confirmation);
      });
    });
  });
});
