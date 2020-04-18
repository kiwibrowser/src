// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for extension-manager unit tests. Unlike
 * extension_manager_test.js, these tests are not interacting with the real
 * chrome.developerPrivate API. */
cr.define('extension_manager_tests', function() {
  /** @enum {string} */
  let TestNames = {
    EnableAndDisable: 'enable and disable',
    ItemOrder: 'item order',
    ProfileSettings: 'profile settings',
    ToggleIncognitoMode: 'toggle incognito mode',
    Uninstall: 'uninstall',
    UninstallFromDetails: 'uninstall while in details view',
    UpdateItemData: 'update item data',
    KioskMode: 'kiosk mode',
  };

  var suiteName = 'ExtensionManagerUnitTest';

  suite(suiteName, function() {
    /** @type {extensions.Manager} */
    let manager;

    /** @type {TestService} */
    let service;

    /** @type {extensions.KioskBrowserProxy} */
    let browserProxy;

    setup(function() {
      if (cr.isChromeOS) {
        browserProxy = new TestKioskBrowserProxy();
        extensions.KioskBrowserProxyImpl.instance_ = browserProxy;
      }

      PolymerTest.clearBody();

      service = new extensions.TestService();
      extensions.Service.instance_ = service;

      manager = document.createElement('extensions-manager');
      document.body.appendChild(manager);

      // Wait until Manager calls fetches data and initializes itself before
      // making any assertions.
      return Promise.all([
        service.whenCalled('getExtensionsInfo'),
        service.whenCalled('getProfileConfiguration'),
      ]);
    });

    /**
     * Trigger an event that indicates that an extension was installed.
     * @param {!chrome.developerPrivate.ExtensionInfo} info
     */
    function simulateExtensionInstall(info) {
      service.itemStateChangedTarget.callListeners({
        event_type: chrome.developerPrivate.EventType.INSTALLED,
        extensionInfo: info,
      });
    }

    // Test that newly added items are inserted in the correct order.
    test(assert(TestNames.ItemOrder), function() {
      expectEquals(0, manager.extensions_.length);

      const alphaFromStore = extension_test_util.createExtensionInfo(
          {location: 'FROM_STORE', name: 'Alpha', id: 'a'.repeat(32)});
      simulateExtensionInstall(alphaFromStore);
      expectEquals(1, manager.extensions_.length);
      expectEquals(alphaFromStore.id, manager.extensions_[0].id);

      // Unpacked extensions come first.
      const betaUnpacked = extension_test_util.createExtensionInfo(
          {location: 'UNPACKED', name: 'Beta', id: 'b'.repeat(32)});
      simulateExtensionInstall(betaUnpacked);
      expectEquals(2, manager.extensions_.length);
      expectEquals(betaUnpacked.id, manager.extensions_[0].id);
      expectEquals(alphaFromStore.id, manager.extensions_[1].id);

      // Extensions from the same location are sorted by name.
      let gammaUnpacked = extension_test_util.createExtensionInfo(
          {location: 'UNPACKED', name: 'Gamma', id: 'c'.repeat(32)});
      simulateExtensionInstall(gammaUnpacked);
      expectEquals(3, manager.extensions_.length);
      expectEquals(betaUnpacked.id, manager.extensions_[0].id);
      expectEquals(gammaUnpacked.id, manager.extensions_[1].id);
      expectEquals(alphaFromStore.id, manager.extensions_[2].id);

      // The name-sort should be case-insensitive, and should fall back on
      // id.
      const aaFromStore = extension_test_util.createExtensionInfo(
          {location: 'FROM_STORE', name: 'AA', id: 'd'.repeat(32)});
      simulateExtensionInstall(aaFromStore);
      const AaFromStore = extension_test_util.createExtensionInfo(
          {location: 'FROM_STORE', name: 'Aa', id: 'e'.repeat(32)});
      simulateExtensionInstall(AaFromStore);
      const aAFromStore = extension_test_util.createExtensionInfo(
          {location: 'FROM_STORE', name: 'aA', id: 'f'.repeat(32)});
      simulateExtensionInstall(aAFromStore);

      expectEquals(6, manager.extensions_.length);
      expectEquals(betaUnpacked.id, manager.extensions_[0].id);
      expectEquals(gammaUnpacked.id, manager.extensions_[1].id);
      expectEquals(aaFromStore.id, manager.extensions_[2].id);
      expectEquals(AaFromStore.id, manager.extensions_[3].id);
      expectEquals(aAFromStore.id, manager.extensions_[4].id);
      expectEquals(alphaFromStore.id, manager.extensions_[5].id);
    });

    test(assert(TestNames.UpdateItemData), function() {
      const oldDescription = 'old description';
      const newDescription = 'new description';

      const extension = extension_test_util.createExtensionInfo(
          {description: oldDescription});
      simulateExtensionInstall(extension);
      const secondExtension = extension_test_util.createExtensionInfo({
        description: 'irrelevant',
        id: 'b'.repeat(32),
      });
      simulateExtensionInstall(secondExtension);

      let data = manager.extensions_[0];
      // The detail view is not present until navigation.
      expectFalse(!!manager.$$('extensions-detail-view'));
      // TODO(scottchen): maybe testing too many things in a single unit test.
      extensions.navigation.navigateTo(
          {page: Page.DETAILS, extensionId: extension.id});
      const detailsView = manager.$$('extensions-detail-view');
      expectTrue(!!detailsView);  // View should now be present.
      expectEquals(extension.id, detailsView.data.id);
      expectEquals(oldDescription, detailsView.data.description);
      expectEquals(
          oldDescription,
          detailsView.$$('.section .section-content').textContent.trim());

      let extensionCopy = Object.assign({}, extension);
      extensionCopy.description = newDescription;
      service.itemStateChangedTarget.callListeners({
        event_type: chrome.developerPrivate.EventType.PREFS_CHANGED,
        extensionInfo: extensionCopy,
      });
      expectEquals(extension.id, detailsView.data.id);
      expectEquals(newDescription, detailsView.data.description);
      expectEquals(
          newDescription,
          detailsView.$$('.section .section-content').textContent.trim());

      // Updating a different extension shouldn't have any impact.
      let secondExtensionCopy = Object.assign({}, secondExtension);
      secondExtensionCopy.description = 'something else';
      service.itemStateChangedTarget.callListeners({
        event_type: chrome.developerPrivate.EventType.PREFS_CHANGED,
        extensionInfo: secondExtensionCopy,
      });
      expectEquals(extension.id, detailsView.data.id);
      expectEquals(newDescription, detailsView.data.description);
      expectEquals(
          newDescription,
          detailsView.$$('.section .section-content').textContent.trim());
    });

    test(assert(TestNames.ProfileSettings), function() {
      expectFalse(manager.inDevMode);

      service.profileStateChangedTarget.callListeners({inDeveloperMode: true});
      expectTrue(manager.inDevMode);

      service.profileStateChangedTarget.callListeners({inDeveloperMode: false});
      expectFalse(manager.inDevMode);

      service.profileStateChangedTarget.callListeners({canLoadUnpacked: true});
      expectTrue(manager.canLoadUnpacked);

      service.profileStateChangedTarget.callListeners({canLoadUnpacked: false});
      expectFalse(manager.canLoadUnpacked);
    });

    test(assert(TestNames.Uninstall), function() {
      expectEquals(0, manager.extensions_.length);

      const extension = extension_test_util.createExtensionInfo(
          {location: 'FROM_STORE', name: 'Alpha', id: 'a'.repeat(32)});
      simulateExtensionInstall(extension);
      expectEquals(1, manager.extensions_.length);

      service.itemStateChangedTarget.callListeners({
        event_type: chrome.developerPrivate.EventType.UNINSTALLED,
        // When an extension is uninstalled, only the ID is passed back from
        // C++.
        item_id: extension.id,
      });

      expectEquals(0, manager.extensions_.length);
    });

    /** @param {string} tagName */
    function assertViewActive(tagName) {
      expectTrue(!!manager.$.viewManager.querySelector(`${tagName}.active`));
    }

    // Test case where an extension is uninstalled from the details view. User
    // should be forwarded to the main view.
    test(assert(TestNames.UninstallFromDetails), function(done) {
      const extension = extension_test_util.createExtensionInfo(
          {location: 'FROM_STORE', name: 'Alpha', id: 'a'.repeat(32)});
      simulateExtensionInstall(extension);

      extensions.navigation.navigateTo(
          {page: Page.DETAILS, extensionId: extension.id});
      Polymer.dom.flush();
      assertViewActive('extensions-detail-view');

      window.addEventListener('popstate', () => {
        assertViewActive('extensions-item-list');
        done();
      });

      service.itemStateChangedTarget.callListeners({
        event_type: chrome.developerPrivate.EventType.UNINSTALLED,
        // When an extension is uninstalled, only the ID is passed back from
        // C++.
        item_id: extension.id,
      });
    });

    test(assert(TestNames.ToggleIncognitoMode), function() {
      expectEquals(0, manager.extensions_.length);
      const extension = extension_test_util.createExtensionInfo(
          {location: 'FROM_STORE', name: 'Alpha', id: 'a'.repeat(32)});
      simulateExtensionInstall(extension);
      expectEquals(1, manager.extensions_.length);

      expectEquals(extension, manager.extensions_[0]);
      expectTrue(extension.incognitoAccess.isEnabled);
      expectFalse(extension.incognitoAccess.isActive);

      // Simulate granting incognito permission.
      const extensionCopy1 = Object.assign({}, extension);
      extensionCopy1.incognitoAccess.isActive = true;
      service.itemStateChangedTarget.callListeners({
        event_type: chrome.developerPrivate.EventType.LOADED,
        extensionInfo: extensionCopy1,
      });

      expectTrue(manager.extensions_[0].incognitoAccess.isActive);

      // Simulate revoking incognito permission.
      const extensionCopy2 = Object.assign({}, extension);
      extensionCopy2.incognitoAccess.isActive = false;
      service.itemStateChangedTarget.callListeners({
        event_type: chrome.developerPrivate.EventType.LOADED,
        extensionInfo: extensionCopy2,
      });
      expectFalse(manager.extensions_[0].incognitoAccess.isActive);
    });

    test(assert(TestNames.EnableAndDisable), function() {
      const ExtensionState = chrome.developerPrivate.ExtensionState;
      expectEquals(0, manager.extensions_.length);
      const extension = extension_test_util.createExtensionInfo(
          {location: 'FROM_STORE', name: 'My extension 1', id: 'a'.repeat(32)});
      simulateExtensionInstall(extension);
      expectEquals(1, manager.extensions_.length);

      expectEquals(extension, manager.extensions_[0]);
      expectEquals('My extension 1', extension.name);
      expectEquals(ExtensionState.ENABLED, extension.state);

      // Simulate disabling an extension.
      const extensionCopy1 = Object.assign({}, extension);
      extensionCopy1.state = ExtensionState.DISABLED;
      service.itemStateChangedTarget.callListeners({
        event_type: chrome.developerPrivate.EventType.LOADED,
        extensionInfo: extensionCopy1,
      });
      expectEquals(ExtensionState.DISABLED, manager.extensions_[0].state);

      // Simulate re-enabling an extension.
      // Simulate disabling an extension.
      const extensionCopy2 = Object.assign({}, extension);
      extensionCopy2.state = ExtensionState.ENABLED;
      service.itemStateChangedTarget.callListeners({
        event_type: chrome.developerPrivate.EventType.LOADED,
        extensionInfo: extensionCopy2,
      });
      expectEquals(ExtensionState.ENABLED, manager.extensions_[0].state);
    });

    if (cr.isChromeOS) {
      test(assert(TestNames.KioskMode), function() {
        expectFalse(!!manager.$$('extensions-kiosk-dialog'));

        return browserProxy.whenCalled('initializeKioskAppSettings')
            .then(() => {
              expectTrue(manager.$$('extensions-toolbar').kioskEnabled);
              manager.$$('extensions-toolbar').fire('kiosk-tap');
              Polymer.dom.flush();
              expectTrue(!!manager.$$('extensions-kiosk-dialog'));
            });
      });
    }
  });

  return {
    suiteName: suiteName,
    TestNames: TestNames,
  };
});
