// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('settings_reset_page', function() {
  /** @enum {string} */
  const TestNames = {
    PowerwashDialogAction: 'PowerwashDialogAction',
    PowerwashDialogOpenClose: 'PowerwashDialogOpenClose',
    ResetProfileDialogAction: 'ResetProfileDialogAction',
    ResetProfileDialogOpenClose: 'ResetProfileDialogOpenClose',
    ResetProfileDialogOriginUnknown: 'ResetProfileDialogOriginUnknown',
    ResetProfileDialogOriginUserClick: 'ResetProfileDialogOriginUserClick',
    ResetProfileDialogOriginTriggeredReset:
        'ResetProfileDialogOriginTriggeredReset',
  };

  function registerDialogTests() {
    suite('DialogTests', function() {
      let resetPage = null;

      /** @type {!settings.ResetPageBrowserProxy} */
      let resetPageBrowserProxy = null;

      /** @type {!settings.LifetimeBrowserProxy} */
      let lifetimeBrowserProxy = null;

      setup(function() {
        if (cr.isChromeOS) {
          lifetimeBrowserProxy = new settings.TestLifetimeBrowserProxy();
          settings.LifetimeBrowserProxyImpl.instance_ = lifetimeBrowserProxy;
        }

        resetPageBrowserProxy = new reset_page.TestResetBrowserProxy();
        settings.ResetBrowserProxyImpl.instance_ = resetPageBrowserProxy;

        PolymerTest.clearBody();
        resetPage = document.createElement('settings-reset-page');
        document.body.appendChild(resetPage);
      });

      teardown(function() {
        resetPage.remove();
      });

      /**
       * @param {function(SettingsResetProfileDialogElement)}
       *     closeDialogFn A function to call for closing the dialog.
       * @return {!Promise}
       */
      function testOpenCloseResetProfileDialog(closeDialogFn) {
        resetPageBrowserProxy.resetResolver('onShowResetProfileDialog');
        resetPageBrowserProxy.resetResolver('onHideResetProfileDialog');

        // Open reset profile dialog.
        MockInteractions.tap(resetPage.$.resetProfile);
        Polymer.dom.flush();
        const dialog = resetPage.$$('settings-reset-profile-dialog');
        assertTrue(!!dialog);
        assertTrue(dialog.$.dialog.open);

        const whenDialogClosed = test_util.eventToPromise('close', dialog);

        return resetPageBrowserProxy.whenCalled('onShowResetProfileDialog')
            .then(function() {
              closeDialogFn(dialog);
              return Promise.all([
                whenDialogClosed,
                resetPageBrowserProxy.whenCalled('onHideResetProfileDialog'),
              ]);
            });
      }

      // Tests that the reset profile dialog opens and closes correctly and that
      // resetPageBrowserProxy calls are occurring as expected.
      test(TestNames.ResetProfileDialogOpenClose, function() {
        return testOpenCloseResetProfileDialog(function(dialog) {
                 // Test case where the 'cancel' button is clicked.
                 MockInteractions.tap(dialog.$.cancel);
               })
            .then(function() {
              return testOpenCloseResetProfileDialog(function(dialog) {
                // Test case where the 'close' button is clicked.
                MockInteractions.tap(dialog.$.dialog.getCloseButton());
              });
            })
            .then(function() {
              return testOpenCloseResetProfileDialog(function(dialog) {
                // Test case where the browser's 'back' button is clicked.
                resetPage.currentRouteChanged(settings.routes.BASIC);
              });
            });
      });

      // Tests that when user request to reset the profile the appropriate
      // message is sent to the browser.
      test(TestNames.ResetProfileDialogAction, function() {
        // Open reset profile dialog.
        MockInteractions.tap(resetPage.$.resetProfile);
        Polymer.dom.flush();
        const dialog = resetPage.$$('settings-reset-profile-dialog');
        assertTrue(!!dialog);

        const checkbox = dialog.$$('[slot=footer] cr-checkbox');
        assertTrue(checkbox.checked);
        const showReportedSettingsLink = dialog.$$('[slot=footer] a');
        assertTrue(!!showReportedSettingsLink);
        MockInteractions.tap(showReportedSettingsLink);

        return resetPageBrowserProxy.whenCalled('showReportedSettings')
            .then(function() {
              // Ensure that the checkbox was not toggled as a result of
              // clicking the link.
              assertTrue(checkbox.checked);
              assertFalse(dialog.$.reset.disabled);
              assertFalse(dialog.$.resetSpinner.active);
              MockInteractions.tap(dialog.$.reset);
              assertTrue(dialog.$.reset.disabled);
              assertTrue(dialog.$.cancel.disabled);
              assertTrue(dialog.$.resetSpinner.active);
              return resetPageBrowserProxy.whenCalled(
                  'performResetProfileSettings');
            });
      });

      function testResetRequestOrigin(expectedOrigin) {
        const dialog = resetPage.$$('settings-reset-profile-dialog');
        assertTrue(!!dialog);
        MockInteractions.tap(dialog.$.reset);
        return resetPageBrowserProxy.whenCalled('performResetProfileSettings')
            .then(function(resetRequest) {
              assertEquals(expectedOrigin, resetRequest);
            });
      }

      test(TestNames.ResetProfileDialogOriginUnknown, function() {
        settings.navigateTo(settings.routes.RESET_DIALOG);
        return resetPageBrowserProxy.whenCalled('onShowResetProfileDialog')
            .then(function() {
              return testResetRequestOrigin('');
            });
      });

      test(TestNames.ResetProfileDialogOriginUserClick, function() {
        MockInteractions.tap(resetPage.$.resetProfile);
        return resetPageBrowserProxy.whenCalled('onShowResetProfileDialog')
            .then(function() {
              return testResetRequestOrigin('userclick');
            });
      });

      test(TestNames.ResetProfileDialogOriginTriggeredReset, function() {
        settings.navigateTo(settings.routes.TRIGGERED_RESET_DIALOG);
        return resetPageBrowserProxy.whenCalled('onShowResetProfileDialog')
            .then(function() {
              return testResetRequestOrigin('triggeredreset');
            });
      });

      if (cr.isChromeOS) {
        /**
         * @param {function(SettingsPowerwashDialogElement):!Element}
         *     closeButtonFn A function that returns the button to be used for
         *     closing the dialog.
         * @return {!Promise}
         */
        function testOpenClosePowerwashDialog(closeButtonFn) {
          // Open powerwash dialog.
          MockInteractions.tap(resetPage.$.powerwash);
          Polymer.dom.flush();
          const dialog = resetPage.$$('settings-powerwash-dialog');
          assertTrue(!!dialog);
          assertTrue(dialog.$.dialog.open);
          const onDialogClosed = new Promise(function(resolve, reject) {
            dialog.addEventListener('close', function() {
              assertFalse(dialog.$.dialog.open);
              resolve();
            });
          });

          MockInteractions.tap(closeButtonFn(dialog));
          return Promise.all([
            onDialogClosed,
            resetPageBrowserProxy.whenCalled('onPowerwashDialogShow'),
          ]);
        }

        // Tests that the powerwash dialog opens and closes correctly, and
        // that chrome.send calls are propagated as expected.
        test(TestNames.PowerwashDialogOpenClose, function() {
          // Test case where the 'cancel' button is clicked.
          return testOpenClosePowerwashDialog(function(dialog) {
                   return dialog.$.cancel;
                 })
              .then(function() {
                // Test case where the 'close' button is clicked.
                return testOpenClosePowerwashDialog(function(dialog) {
                  return dialog.$.dialog.getCloseButton();
                });
              });
        });

        // Tests that when powerwash is requested chrome.send calls are
        // propagated as expected.
        test(TestNames.PowerwashDialogAction, function() {
          // Open powerwash dialog.
          MockInteractions.tap(resetPage.$.powerwash);
          Polymer.dom.flush();
          const dialog = resetPage.$$('settings-powerwash-dialog');
          assertTrue(!!dialog);
          MockInteractions.tap(dialog.$.powerwash);
          return lifetimeBrowserProxy.whenCalled('factoryReset')
              .then((requestTpmFirmwareUpdate) => {
                assertFalse(requestTpmFirmwareUpdate);
              });
        });
      }
    });
  }

  registerDialogTests();
});
