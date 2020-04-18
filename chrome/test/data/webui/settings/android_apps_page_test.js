// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @type {?SettingsAndroidAppsPageElement} */
let androidAppsPage = null;

/** @type {?TestAndroidAppsBrowserProxy} */
let androidAppsBrowserProxy = null;

const setAndroidAppsState = function(playStoreEnabled, settingsAppAvailable) {
  const appsInfo = {
    playStoreEnabled: playStoreEnabled,
    settingsAppAvailable: settingsAppAvailable,
  };
  androidAppsPage.androidAppsInfo = appsInfo;
  Polymer.dom.flush();
};

suite('AndroidAppsPageTests', function() {
  setup(function() {
    androidAppsBrowserProxy = new TestAndroidAppsBrowserProxy();
    settings.AndroidAppsBrowserProxyImpl.instance_ = androidAppsBrowserProxy;
    PolymerTest.clearBody();
    androidAppsPage = document.createElement('settings-android-apps-page');
    document.body.appendChild(androidAppsPage);
    testing.Test.disableAnimationsAndTransitions();
  });

  teardown(function() {
    androidAppsPage.remove();
  });

  teardown(function() {
    androidAppsPage.remove();
  });

  suite('Main Page', function() {
    setup(function() {
      androidAppsPage.havePlayStoreApp = true;
      androidAppsPage.prefs = {arc: {enabled: {value: false}}};
      setAndroidAppsState(false, false);
    });

    test('Enable', function() {
      const button = androidAppsPage.$$('#enable');
      assertTrue(!!button);
      assertFalse(!!androidAppsPage.$$('.subpage-arrow'));

      MockInteractions.tap(button);
      Polymer.dom.flush();
      assertTrue(androidAppsPage.prefs.arc.enabled.value);

      setAndroidAppsState(true, false);
      assertTrue(!!androidAppsPage.$$('.subpage-arrow'));
    });
  });

  suite('SubPage', function() {
    let subpage;

    function flushAsync() {
      Polymer.dom.flush();
      return new Promise(resolve => {
        androidAppsPage.async(resolve);
      });
    }

    /**
     * Returns a new promise that resolves after a window 'popstate' event.
     * @return {!Promise}
     */
    function whenPopState() {
      return new Promise(function(resolve) {
        window.addEventListener('popstate', function callback() {
          window.removeEventListener('popstate', callback);
          resolve();
        });
      });
    }

    setup(function() {
      androidAppsPage.havePlayStoreApp = true;
      androidAppsPage.prefs = {arc: {enabled: {value: true}}};
      setAndroidAppsState(true, false);
      settings.navigateTo(settings.routes.ANDROID_APPS);
      MockInteractions.tap(androidAppsPage.$$('#android-apps'));
      return flushAsync().then(() => {
        subpage = androidAppsPage.$$('settings-android-apps-subpage');
        assertTrue(!!subpage);
      });
    });

    test('Sanity', function() {
      assertTrue(!!subpage.$$('#remove'));
      assertTrue(!subpage.$$('settings-android-settings-element'));
    });

    test('ManageAppsUpdate', function() {
      assertTrue(!subpage.$$('settings-android-settings-element'));
      setAndroidAppsState(true, true);
      assertTrue(!!subpage.$$('settings-android-settings-element'));
      assertTrue(
          !!subpage.$$('settings-android-settings-element').$$('#manageApps'));
      setAndroidAppsState(true, false);
      assertTrue(!subpage.$$('settings-android-settings-element'));
    });

    test('ManageAppsOpenRequest', function() {
      setAndroidAppsState(true, true);
      const button =
          subpage.$$('settings-android-settings-element').$$('#manageApps');
      assertTrue(!!button);
      const promise =
          androidAppsBrowserProxy.whenCalled('showAndroidAppsSettings');
      // MockInteractions.tap does not work here due style is not updated.
      button.click();
      Polymer.dom.flush();
      return promise;
    });

    test('Disable', function() {
      const dialog = subpage.$$('#confirmDisableDialog');
      assertTrue(!!dialog);
      assertFalse(dialog.open);

      const remove = subpage.$$('#remove');
      assertTrue(!!remove);

      subpage.onRemoveTap_();
      Polymer.dom.flush();
      assertTrue(dialog.open);
      dialog.close();
    });

    test('HideOnDisable', function() {
      assertEquals(
          settings.getCurrentRoute(), settings.routes.ANDROID_APPS_DETAILS);
      setAndroidAppsState(false, false);
      return whenPopState().then(function() {
        assertEquals(settings.getCurrentRoute(), settings.routes.ANDROID_APPS);
      });
    });
  });

  suite('Enforced', function() {
    let subpage;

    setup(function() {
      androidAppsPage.havePlayStoreApp = true;
      androidAppsPage.prefs = {
        arc: {
          enabled: {
            value: true,
            enforcement: chrome.settingsPrivate.Enforcement.ENFORCED
          }
        }
      };
      setAndroidAppsState(true, true);
      MockInteractions.tap(androidAppsPage.$$('#android-apps'));
      Polymer.dom.flush();
      subpage = androidAppsPage.$$('settings-android-apps-subpage');
      assertTrue(!!subpage);
    });

    test('Sanity', function() {
      Polymer.dom.flush();
      assertFalse(!!subpage.$$('#remove'));
      assertTrue(!!subpage.$$('settings-android-settings-element'));
      assertTrue(
          !!subpage.$$('settings-android-settings-element').$$('#manageApps'));
    });
  });

  suite('NoPlayStore', function() {
    setup(function() {
      androidAppsPage.havePlayStoreApp = false;
      androidAppsPage.prefs = {arc: {enabled: {value: true}}};
      setAndroidAppsState(true, true);
    });

    test('Sanity', function() {
      assertTrue(!!androidAppsPage.$$('settings-android-settings-element'));
      assertTrue(!!androidAppsPage.$$('settings-android-settings-element')
                       .$$('#manageApps'));
    });

    test('ManageAppsOpenRequest', function() {
      const button = androidAppsPage.$$('settings-android-settings-element')
                         .$$('#manageApps');
      assertTrue(!!button);
      const promise =
          androidAppsBrowserProxy.whenCalled('showAndroidAppsSettings');
      // MockInteractions.tap does not work here due style is not updated.
      button.click();
      Polymer.dom.flush();
      return promise;
    });
  });

});
