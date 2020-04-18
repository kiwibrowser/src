// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

suite('InternetDetailPage', function() {
  /** @type {InternetDetailPageElement} */
  let internetDetailPage = null;

  /** @type {NetworkingPrivate} */
  let api_;

  suiteSetup(function() {
    loadTimeData.overrideValues({
      internetAddConnection: 'internetAddConnection',
      internetAddConnectionExpandA11yLabel:
          'internetAddConnectionExpandA11yLabel',
      internetAddConnectionNotAllowed: 'internetAddConnectionNotAllowed',
      internetAddThirdPartyVPN: 'internetAddThirdPartyVPN',
      internetAddVPN: 'internetAddVPN',
      internetAddArcVPN: 'internetAddArcVPN',
      internetAddArcVPNProvider: 'internetAddArcVPNProvider',
      internetAddWiFi: 'internetAddWiFi',
      internetDetailPageTitle: 'internetDetailPageTitle',
      internetKnownNetworksPageTitle: 'internetKnownNetworksPageTitle',
    });

    CrOncStrings = {
      OncTypeCellular: 'OncTypeCellular',
      OncTypeEthernet: 'OncTypeEthernet',
      OncTypeTether: 'OncTypeTether',
      OncTypeVPN: 'OncTypeVPN',
      OncTypeWiFi: 'OncTypeWiFi',
      OncTypeWiMAX: 'OncTypeWiMAX',
      networkListItemConnected: 'networkListItemConnected',
      networkListItemConnecting: 'networkListItemConnecting',
      networkListItemConnectingTo: 'networkListItemConnectingTo',
      networkListItemNotConnected: 'networkListItemNotConnected',
      networkListItemNoNetwork: 'networkListItemNoNetwork',
      vpnNameTemplate: 'vpnNameTemplate',
    };

    api_ = new chrome.FakeNetworkingPrivate();

    // Disable animations so sub-pages open within one event loop.
    testing.Test.disableAnimationsAndTransitions();
  });

  function flushAsync() {
    Polymer.dom.flush();
    return new Promise(resolve => {
      internetDetailPage.async(resolve);
    });
  }

  function setNetworksForTest(networks) {
    api_.resetForTest();
    api_.addNetworksForTest(networks);
  }

  function getAllowSharedProxy() {
    const proxySection = internetDetailPage.$$('network-proxy-section');
    assertTrue(!!proxySection);
    const allowShared = proxySection.$$('#allowShared');
    assertTrue(!!allowShared);
    return allowShared;
  }

  setup(function() {
    PolymerTest.clearBody();
    internetDetailPage =
        document.createElement('settings-internet-detail-page');
    assertTrue(!!internetDetailPage);
    api_.resetForTest();
    internetDetailPage.networkingPrivate = api_;
    document.body.appendChild(internetDetailPage);
    return flushAsync();
  });

  teardown(function() {
    internetDetailPage.remove();
    delete internetDetailPage;
    settings.resetRouteForTesting();
  });

  suite('DetailsPage', function() {
    test('WiFi', function() {
      api_.enableNetworkType('WiFi');
      setNetworksForTest([{GUID: 'wifi1_guid', Name: 'wifi1', Type: 'WiFi'}]);
      internetDetailPage.init('wifi1_guid', 'WiFi', 'wifi1');
      assertEquals('wifi1_guid', internetDetailPage.guid);
      return flushAsync().then(() => {
        return Promise.all([
          api_.whenCalled('getManagedProperties'),
        ]);
      });
    });

    test('Proxy Unshared', function() {
      api_.enableNetworkType('WiFi');
      setNetworksForTest([{
        GUID: 'wifi_user_guid',
        Name: 'wifi_user',
        Type: 'WiFi',
        Source: 'User'
      }]);
      internetDetailPage.init('wifi_user_guid', 'WiFi', 'wifi_user');
      return flushAsync().then(() => {
        const proxySection = internetDetailPage.$$('network-proxy-section');
        assertTrue(!!proxySection);
        const allowShared = proxySection.$$('#allowShared');
        assertTrue(!!allowShared);
        assertTrue(allowShared.hasAttribute('hidden'));
      });
    });

    test('Proxy Shared', function() {
      api_.enableNetworkType('WiFi');
      setNetworksForTest([{
        GUID: 'wifi_user_guid',
        Name: 'wifi_user',
        Type: 'WiFi',
        Source: 'Device'
      }]);
      internetDetailPage.init('wifi_user_guid', 'WiFi', 'wifi_user');
      return flushAsync().then(() => {
        let allowShared = getAllowSharedProxy();
        assertFalse(allowShared.hasAttribute('hidden'));
        assertFalse(allowShared.disabled);
      });
    });

    // When proxy settings are managed by a user policy they may respect the
    // allowd_shared_proxies pref so #allowShared should be visible.
    // TOD(stevenjb): Improve this: crbug.com/662529.
    test('Proxy Shared User Managed', function() {
      api_.enableNetworkType('WiFi');
      setNetworksForTest([{
        GUID: 'wifi_user_guid',
        Name: 'wifi_user',
        Type: 'WiFi',
        Source: 'Device',
        ProxySettings: {
          Type: {
            Active: 'Manual',
            Effective: 'UserPolicy',
            UserEditable: false
          }
        }
      }]);
      internetDetailPage.init('wifi_user_guid', 'WiFi', 'wifi_user');
      return flushAsync().then(() => {
        let allowShared = getAllowSharedProxy();
        assertFalse(allowShared.hasAttribute('hidden'));
        assertFalse(allowShared.disabled);
      });
    });

    // When proxy settings are managed by a device policy they may respect the
    // allowd_shared_proxies pref so #allowShared should be visible.
    test('Proxy Shared Device Managed', function() {
      api_.enableNetworkType('WiFi');
      setNetworksForTest([{
        GUID: 'wifi_user_guid',
        Name: 'wifi_user',
        Type: 'WiFi',
        Source: 'Device',
        ProxySettings: {
          Type: {
            Active: 'Manual',
            Effective: 'DevicePolicy',
            DeviceEditable: false
          }
        }
      }]);
      internetDetailPage.init('wifi_user_guid', 'WiFi', 'wifi_user');
      return flushAsync().then(() => {
        let allowShared = getAllowSharedProxy();
        assertFalse(allowShared.hasAttribute('hidden'));
        assertFalse(allowShared.disabled);
      });
    });
  });
});
