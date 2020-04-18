// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for usb_devices. */
suite('UsbDevices', function() {
  /**
   * A dummy usb-devices element created before each test.
   * @type {UsbDevices}
   */
  let testElement;

  /**
   * The mock proxy object to use during test.
   * @type {TestSiteSettingsPrefsBrowserProxy}
   */
  let browserProxy = null;

  /**
   * An example USB device entry list.
   * @type {!Array<UsbDeviceEntry>}
   */
  const deviceList = [
    {
      embeddingOrigin: 'device-1-embedding-origin',
      object: {
        name: 'device-1',
        'product-id': 1,
        'serial-number': 'device-1-sn',
        'vendor-id': 1
      },
      objectName: 'device-1',
      origin: 'device-1-origin',
      setting: 'device-1-settings',
      source: 'device-1-source'
    },
    {
      embeddingOrigin: 'device-2-embedding-origin',
      object: {
        name: 'device-2',
        'product-id': 2,
        'serial-number': 'device-2-sn',
        'vendor-id': 2
      },
      objectName: 'device-2',
      origin: 'device-2-origin',
      setting: 'device-2-settings',
      source: 'device-2-source'
    }
  ];

  setup(function() {
    browserProxy = new TestSiteSettingsPrefsBrowserProxy();
    settings.SiteSettingsPrefsBrowserProxyImpl.instance_ = browserProxy;
  });

  teardown(function() {
    testElement.remove();
    testElement = null;
  });

  /** @return {!Promise} */
  function initPage() {
    browserProxy.reset();
    PolymerTest.clearBody();
    testElement = document.createElement('usb-devices');
    document.body.appendChild(testElement);
    return browserProxy.whenCalled('fetchUsbDevices').then(function() {
      Polymer.dom.flush();
    });
  }

  test('empty devices list', function() {
    return initPage().then(function() {
      const listItems = testElement.root.querySelectorAll('.list-item');
      assertEquals(0, listItems.length);
    });
  });

  test('non-empty device list', function() {
    browserProxy.setUsbDevices(deviceList);

    return initPage().then(function() {
      const listItems = testElement.root.querySelectorAll('.list-item');
      assertEquals(deviceList.length, listItems.length);
    });
  });

  test('non-empty device list has working menu buttons', function() {
    browserProxy.setUsbDevices(deviceList);

    return initPage().then(function() {
      const menuButton =
          testElement.$$('paper-icon-button-light.icon-more-vert');
      assertTrue(!!menuButton);
      MockInteractions.tap(menuButton.querySelector('button'));
      const dialog = testElement.$$('cr-action-menu');
      assertTrue(dialog.open);
    });
  });

  /**
   * A reusable function to test removing different devices.
   * @param {!number} indexToRemove index of devices to be removed.
   * @return {!Promise}
   */
  function testRemovalFlow(indexToRemove) {
    /**
     * Test whether or not clicking remove-button sends the correct
     * parameters to the browserProxy.removeUsbDevice() function.
     */
    const menuButton = testElement.root.querySelectorAll(
        'paper-icon-button-light.icon-more-vert')[indexToRemove];
    const removeButton = testElement.$.removeButton;
    MockInteractions.tap(menuButton.querySelector('button'));
    MockInteractions.tap(removeButton);
    return browserProxy.whenCalled('removeUsbDevice').then(function(args) {
      /**
       * removeUsbDevice() is expected to be called with arguments as
       * [origin, embeddingOrigin, object].
       */
      assertEquals(deviceList[indexToRemove].origin, args[0]);
      assertEquals(deviceList[indexToRemove].embeddingOrigin, args[1]);
      assertEquals(deviceList[indexToRemove].object, args[2]);

      const dialog = testElement.$$('cr-action-menu');
      assertFalse(dialog.open);
    });
  }

  test('try removing items using remove button', function() {
    browserProxy.setUsbDevices(deviceList);

    const self = this;

    return initPage()
        .then(function() {
          return testRemovalFlow(0);
        })
        .then(function() {
          browserProxy.reset();
          return testRemovalFlow(1);
        });
  });
});
