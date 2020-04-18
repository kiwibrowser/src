// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for Easy Unlock within People section. */

GEN_INCLUDE(['settings_page_browsertest.js']);

/**
 * @constructor
 * @extends {SettingsPageBrowserTest}
 */
function SettingsEasyUnlockBrowserTest() {}

SettingsEasyUnlockBrowserTest.prototype = {
  __proto__: SettingsPageBrowserTest.prototype,

  /** @override */
  extraLibraries: PolymerTest.getLibraries(ROOT_PATH).concat([
    '../test_browser_proxy.js',
  ]),
};

// Times out on debug builders and may time out on memory bots because
// the Settings page can take several seconds to load in a Release build
// and several times that in a Debug build. See https://crbug.com/558434
// and http://crbug.com/711256.

// Runs easy unlock tests.
TEST_F('SettingsEasyUnlockBrowserTest', 'DISABLED_EasyUnlock', function() {
  /**
   * A test version of EasyUnlockBrowserProxy. Provides helper methods
   * for allowing tests to know when a method was called, as well as
   * specifying mock responses.
   *
   * @implements {settings.EasyUnlockBrowserProxy}
   */
  class TestEasyUnlockBrowserProxy extends TestBrowserProxy {
    constructor() {
      super([
        'getEnabledStatus',
        'startTurnOnFlow',
        'getTurnOffFlowStatus',
        'startTurnOffFlow',
        'cancelTurnOffFlow',
      ]);

      /** @private {boolean} */
      this.isEnabled_ = false;
    }

    /**
     * @param {boolean} easyUnlockEnabled
     */
    setEnabledStatus(easyUnlockEnabled) {
      this.isEnabled_ = easyUnlockEnabled;
    }

    /** @override */
    getEnabledStatus() {
      this.methodCalled('getEnabledStatus');
      return Promise.resolve(this.isEnabled_);
    }

    /** @override */
    startTurnOnFlow() {
      this.methodCalled('startTurnOnFlow');
    }

    /** @override */
    getTurnOffFlowStatus() {
      this.methodCalled('getTurnOffFlowStatus');
      return Promise.resolve('idle');
    }

    /** @override */
    startTurnOffFlow() {
      this.methodCalled('startTurnOffFlow');
    }

    /** @override */
    cancelTurnOffFlow() {
      this.methodCalled('cancelTurnOffFlow');
    }
  }

  /** @type {?SettingsLockScreenElement} */
  let lockScreen = null;

  /** @type {?TestEasyUnlockBrowserProxy} */
  let browserProxy = null;

  suite('SettingsEasyUnlock', function() {
    suiteSetup(function() {
      Polymer.dom.flush();

      // These overrides are necessary for this test to function on ChromeOS
      // bots that do not have Bluetooth (don't actually support Easy Unlock).
      loadTimeData.overrideValues({
        easyUnlockAllowed: true,

        easyUnlockSectionTitle: '',
        easyUnlockLearnMoreURL: '',
        easyUnlockSetupIntro: '',
        easyUnlockSetupButton: '',

        easyUnlockDescription: '',
        easyUnlockTurnOffTitle: '',
        easyUnlockTurnOffDescription: '',
        easyUnlockTurnOffButton: '',
      });
    });

    setup(function() {
      browserProxy = new TestEasyUnlockBrowserProxy();
      settings.EasyUnlockBrowserProxyImpl.instance_ = browserProxy;

      PolymerTest.clearBody();
      lockScreen = document.createElement('settings-lock-screen');
    });

    test('setup button', function() {
      document.body.appendChild(lockScreen);

      return browserProxy.whenCalled('getEnabledStatus').then(function() {
        assertTrue(lockScreen.easyUnlockAllowed_);
        expectFalse(lockScreen.easyUnlockEnabled_);

        Polymer.dom.flush();

        const setupButton = lockScreen.$$('#easyUnlockSetup');
        assertTrue(!!setupButton);
        expectFalse(setupButton.hidden);

        MockInteractions.tap(setupButton);
        return browserProxy.whenCalled('startTurnOnFlow');
      });
    });

    test('turn off dialog', function() {
      browserProxy.setEnabledStatus(true);
      document.body.appendChild(lockScreen);

      let turnOffDialog = null;

      return browserProxy.whenCalled('getEnabledStatus')
          .then(function() {
            assertTrue(lockScreen.easyUnlockAllowed_);
            expectTrue(lockScreen.easyUnlockEnabled_);

            Polymer.dom.flush();

            const turnOffButton = lockScreen.$$('#easyUnlockTurnOff');
            assertTrue(!!turnOffButton);
            expectFalse(turnOffButton.hidden);

            MockInteractions.tap(turnOffButton);
            return browserProxy.whenCalled('getTurnOffFlowStatus');
          })
          .then(function() {
            Polymer.dom.flush();

            turnOffDialog = lockScreen.$$('#easyUnlockTurnOffDialog');
            assertTrue(!!turnOffDialog);

            // Verify that elements on the turn off dialog are hidden or active
            // according to the easy unlock turn off status.
            const turnOffDialogButtonContainer =
                turnOffDialog.$$('[slot=button-container]');
            const turnOffDialogButtonSpinner =
                turnOffDialog.$$('paper-spinner-lite');
            const turnOffDialogConfirmButton = turnOffDialog.$$('#turnOff');
            const turnOffDialogCancelButton =
                turnOffDialog.$$('.cancel-button');
            assertTrue(!!turnOffDialogButtonContainer);
            assertTrue(!!turnOffDialogButtonSpinner);
            assertTrue(!!turnOffDialogConfirmButton);
            assertTrue(!!turnOffDialogCancelButton);

            cr.webUIListenerCallback(
                'easy-unlock-turn-off-flow-status', 'offline');
            expectTrue(turnOffDialogButtonContainer.hidden);
            expectFalse(turnOffDialogButtonSpinner.active);

            cr.webUIListenerCallback(
                'easy-unlock-turn-off-flow-status', 'pending');
            expectFalse(turnOffDialogButtonContainer.hidden);
            expectTrue(turnOffDialogButtonSpinner.active);

            cr.webUIListenerCallback(
                'easy-unlock-turn-off-flow-status', 'server-error');
            expectFalse(turnOffDialogButtonContainer.hidden);
            expectTrue(turnOffDialogCancelButton.hidden);

            cr.webUIListenerCallback(
                'easy-unlock-turn-off-flow-status', 'idle');
            expectFalse(turnOffDialogConfirmButton.hidden);

            MockInteractions.tap(turnOffDialogConfirmButton);

            return browserProxy.whenCalled('startTurnOffFlow');
          })
          .then(function() {
            // To signal successful turnoff, the enabled status is broadcast
            // as false. At that point, the dialog should close and cancel
            // any in-progress turnoff flow. The cancellation should be
            // a no-op assuming the turnoff originated from this tab.
            cr.webUIListenerCallback('easy-unlock-enabled-status', false);
            return browserProxy.whenCalled('cancelTurnOffFlow');
          })
          .then(function() {
            Polymer.dom.flush();
            expectFalse(turnOffDialog.$.dialog.open);
          });
    });
  });

  // Run all registered tests.
  mocha.run();
});
