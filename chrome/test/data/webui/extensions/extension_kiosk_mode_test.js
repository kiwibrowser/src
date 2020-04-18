// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for extension-kiosk-dialog. */
cr.define('extension_kiosk_mode_tests', function() {
  /** @enum {string} */
  let TestNames = {
    AddButton: 'AddButton',
    AddError: 'AddError',
    AutoLaunch: 'AutoLaunch',
    Bailout: 'Bailout',
    Layout: 'Layout',
    Updated: 'Updated',
  };


  let suiteName = 'kioskModeTests';

  suite(suiteName, function() {

    /** @type {extensions.KioskBrowserProxy} */
    let browserProxy;

    /** @type {extensions.KioskDialog} */
    let dialog;

    /** @type {!Array<!KioskApp>} */
    const basicApps = [
      {
        id: 'app_1',
        name: 'App1 Name',
        iconURL: '',
        autoLaunch: false,
        isLoading: false,
      },
      {
        id: 'app_2',
        name: 'App2 Name',
        iconURL: '',
        autoLaunch: false,
        isLoading: false,
      },
    ];

    /** @param {!KioskAppSettings} */
    function setAppSettings(settings) {
      const appSettings = {
        apps: [],
        disableBailout: false,
        hasAutoLaunchApp: false,
      };

      browserProxy.setAppSettings(Object.assign({}, appSettings, settings));
    }

    /** @param {!KioskSettings} */
    function setInitialSettings(settings) {
      const initialSettings = {
        kioskEnabled: true,
        autoLaunchEnabled: false,
      };

      browserProxy.setInitialSettings(
          Object.assign({}, initialSettings, settings));
    }

    /** @return {!Promise} */
    function initPage() {
      PolymerTest.clearBody();
      browserProxy.reset();
      dialog = document.createElement('extensions-kiosk-dialog');
      document.body.appendChild(dialog);

      return browserProxy.whenCalled('getKioskAppSettings')
          .then(() => PolymerTest.flushTasks());
    }

    setup(function() {
      browserProxy = new TestKioskBrowserProxy();
      setAppSettings({apps: basicApps.slice(0)});
      extensions.KioskBrowserProxyImpl.instance_ = browserProxy;

      return initPage();
    });

    test(assert(TestNames.Layout), function() {
      const apps = basicApps.slice(0);
      apps[1].autoLaunch = true;
      apps[1].isLoading = true;
      setAppSettings({apps: apps, hasAutoLaunchApp: true});

      return initPage()
          .then(() => {
            const items = dialog.shadowRoot.querySelectorAll('.list-item');
            expectEquals(items.length, 2);
            expectTrue(items[0].textContent.includes(basicApps[0].name));
            expectTrue(items[1].textContent.includes(basicApps[1].name));
            // Second item should show the auto-lauch label.
            expectTrue(items[0].querySelector('span').hidden);
            expectFalse(items[1].querySelector('span').hidden);
            // No permission to edit auto-launch so buttons should be hidden.
            expectTrue(items[0].querySelector('paper-button').hidden);
            expectTrue(items[1].querySelector('paper-button').hidden);
            // Bailout checkbox should be hidden when auto-launch editing
            // disabled.
            expectTrue(dialog.$$('cr-checkbox').hidden);

            MockInteractions.tap(
                items[0].querySelector('.icon-delete-gray button'));
            Polymer.dom.flush();
            return browserProxy.whenCalled('removeKioskApp');
          })
          .then(appId => {
            expectEquals(appId, basicApps[0].id);
          });
    });

    test(assert(TestNames.AutoLaunch), function() {
      const apps = basicApps.slice(0);
      apps[1].autoLaunch = true;
      setAppSettings({apps: apps, hasAutoLaunchApp: true});
      setInitialSettings({autoLaunchEnabled: true});

      let buttons;
      return initPage()
          .then(() => {
            buttons =
                dialog.shadowRoot.querySelectorAll('.list-item paper-button');
            // Has permission to edit auto-launch so buttons should be seen.
            expectFalse(buttons[0].hidden);
            expectFalse(buttons[1].hidden);

            MockInteractions.tap(buttons[0]);
            return browserProxy.whenCalled('enableKioskAutoLaunch');
          })
          .then(appId => {
            expectEquals(appId, basicApps[0].id);

            MockInteractions.tap(buttons[1]);
            return browserProxy.whenCalled('disableKioskAutoLaunch');
          })
          .then(appId => {
            expectEquals(appId, basicApps[1].id);
          });
    });

    test(assert(TestNames.Bailout), function() {
      const apps = basicApps.slice(0);
      apps[1].autoLaunch = true;
      setAppSettings({apps: apps, hasAutoLaunchApp: true});
      setInitialSettings({autoLaunchEnabled: true});

      expectFalse(dialog.$['confirm-dialog'].open);

      let bailoutCheckbox;
      return initPage()
          .then(() => {
            bailoutCheckbox = dialog.$$('cr-checkbox');
            // Bailout checkbox should be usable when auto-launching.
            expectFalse(bailoutCheckbox.hidden);
            expectFalse(bailoutCheckbox.disabled);
            expectFalse(bailoutCheckbox.checked);

            // Making sure canceling doesn't change anything.
            bailoutCheckbox.click();
            Polymer.dom.flush();
            expectTrue(dialog.$['confirm-dialog'].open);

            MockInteractions.tap(
                dialog.$['confirm-dialog'].querySelector('.cancel-button'));
            Polymer.dom.flush();
            expectFalse(bailoutCheckbox.checked);
            expectFalse(dialog.$['confirm-dialog'].open);
            expectTrue(dialog.$.dialog.open);

            // Accepting confirmation dialog should trigger browserProxy call.
            bailoutCheckbox.click();
            Polymer.dom.flush();
            expectTrue(dialog.$['confirm-dialog'].open);

            MockInteractions.tap(
                dialog.$['confirm-dialog'].querySelector('.action-button'));
            Polymer.dom.flush();
            expectTrue(bailoutCheckbox.checked);
            expectFalse(dialog.$['confirm-dialog'].open);
            expectTrue(dialog.$.dialog.open);
            return browserProxy.whenCalled('setDisableBailoutShortcut');
          })
          .then(disabled => {
            expectTrue(disabled);

            // Test clicking on checkbox again should simply re-enable bailout.
            browserProxy.reset();
            bailoutCheckbox.click();
            expectFalse(bailoutCheckbox.checked);
            expectFalse(dialog.$['confirm-dialog'].open);
            return browserProxy.whenCalled('setDisableBailoutShortcut');
          })
          .then(disabled => {
            expectFalse(disabled);
          });
    });

    test(assert(TestNames.AddButton), function() {
      const addButton = dialog.$['add-button'];
      expectTrue(!!addButton);
      expectTrue(addButton.disabled);

      const addInput = dialog.$['add-input'];
      addInput.value = 'blah';
      expectFalse(addButton.disabled);

      MockInteractions.tap(addButton);
      return browserProxy.whenCalled('addKioskApp').then(appId => {
        expectEquals(appId, 'blah');
      });
    });

    test(assert(TestNames.Updated), function() {
      const items = dialog.shadowRoot.querySelectorAll('.list-item');
      expectTrue(items[0].textContent.includes(basicApps[0].name));

      const newName = 'completely different name';

      cr.webUIListenerCallback('kiosk-app-updated', {
        id: basicApps[0].id,
        name: newName,
        iconURL: '',
        autoLaunch: false,
        isLoading: false,
      });

      expectFalse(items[0].textContent.includes(basicApps[0].name));
      expectTrue(items[0].textContent.includes(newName));
    });

    test(assert(TestNames.AddError), function() {
      const addInput = dialog.$['add-input'];

      expectFalse(!!addInput.invalid);
      cr.webUIListenerCallback('kiosk-app-error', basicApps[0].id);

      expectTrue(!!addInput.invalid);
      expectTrue(addInput.errorMessage.includes(basicApps[0].id));
    });
  });

  return {
    suiteName: suiteName,
    TestNames: TestNames,
  };
});
