// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Suite of tests for page-specific behaviors of
 * SetupSucceededPage.
 */
cr.define('multidevice_setup', () => {
  function registerSetupSucceededPageTests() {
    suite('MultiDeviceSetup', () => {
      /**
       * SetupSucceededPage created before each test. Defined in setUp.
       * @type {SetupSucceededPage|undefined}
       */
      let setupSucceededPageElement;

      const SUCCESS = 'setup-succeeded-page';

      setup(() => {
        setupSucceededPageElement =
            document.createElement('setup-succeeded-page');
        document.body.appendChild(setupSucceededPageElement);
      });

      test('Settings link opens settings page', done => {
        setupSucceededPageElement.addEventListener(
            'settings-opened', () => done());
        let settingsLink = setupSucceededPageElement.$$('#settings-link');
        settingsLink.click();
      });
    });
  }
  return {registerSetupSucceededPageTests: registerSetupSucceededPageTests};
});
