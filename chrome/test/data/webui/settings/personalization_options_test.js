// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('settings_personalization_options', function() {
  function registerTests() {
    suite('SafeBrowsingExtendedReporting', function() {
      /** @type {settings.TestPrivacyPageBrowserProxy} */
      let testBrowserProxy;

      /** @type {SettingsPersonalizationOptionsElement} */
      let testElement;

      setup(function() {
        testBrowserProxy = new TestPrivacyPageBrowserProxy();
        settings.PrivacyPageBrowserProxyImpl.instance_ = testBrowserProxy;
        PolymerTest.clearBody();
        testElement =
            document.createElement('settings-personalization-options');
        document.body.appendChild(testElement);
      });

      teardown(function() {
        testElement.remove();
      });

      test('test whether extended reporting is enabled/managed', function() {
        return testBrowserProxy.whenCalled('getSafeBrowsingExtendedReporting')
            .then(function() {
              Polymer.dom.flush();

              // Control starts checked and managed by default.
              assertTrue(testBrowserProxy.sberPrefState.enabled);
              assertTrue(testBrowserProxy.sberPrefState.managed);

              const control =
                  testElement.$$('#safeBrowsingExtendedReportingControl');
              assertEquals(true, control.checked);
              assertEquals(true, !!control.pref.controlledBy);

              // Change the managed and checked states
              const changedPrefState = {
                enabled: false,
                managed: false,
              };
              // Notification from browser can uncheck the box and make it not
              // managed.
              cr.webUIListenerCallback(
                  'safe-browsing-extended-reporting-change', changedPrefState);
              Polymer.dom.flush();
              assertEquals(false, control.checked);
              assertEquals(false, !!control.pref.controlledBy);

              // Tapping on the box will check it again.
              MockInteractions.tap(control);

              return testBrowserProxy.whenCalled(
                  'setSafeBrowsingExtendedReportingEnabled');
            })
            .then(function(enabled) {
              assertTrue(enabled);
            });
      });
    });
  }

  function registerOfficialBuildTests() {
    suite('SafeBrowsingExtendedReportingOfficialBuild', function() {
      /** @type {settings.TestPrivacyPageBrowserProxy} */
      let testBrowserProxy;

      /** @type {SettingsPersonalizationOptionsElement} */
      let testElement;

      setup(function() {
        testBrowserProxy = new TestPrivacyPageBrowserProxy();
        settings.PrivacyPageBrowserProxyImpl.instance_ = testBrowserProxy;
        PolymerTest.clearBody();
        testElement =
            document.createElement('settings-personalization-options');
        document.body.appendChild(testElement);
      });

      teardown(function() {
        testElement.remove();
      });

      test('displaying toggles depending on unified consent', function() {
        testElement.unifiedConsentEnabled = false;
        Polymer.dom.flush();
        assertEquals(
            7,
            testElement.root.querySelectorAll('settings-toggle-button').length);
        testElement.unifiedConsentEnabled = true;
        Polymer.dom.flush();
        // #spellCheckControl should be set to display: none by false dom-if.
        assertTrue(
            testElement.$$('#spellCheckControl').style.display === 'none');
        assertTrue(!!testElement.$$('#spellCheckLinkBox'));
        assertTrue(
            testElement.$$('#spellCheckLinkBox').style.display !== 'none');
      });
    });
  }

  return {
    registerTests: registerTests,
    registerOfficialBuildTests: registerOfficialBuildTests,
  };
});
