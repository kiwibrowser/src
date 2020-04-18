// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @implements {settings.SmbBrowserProxy} */
class TestSmbBrowserProxy extends TestBrowserProxy {
  constructor() {
    super([
      'smbMount',
    ]);
  }

  /** @override */
  smbMount(smbUrl, username, password) {
    this.methodCalled('smbMount', [smbUrl, username, password]);
  }
}

suite('AddSmbShareDialogTests', function() {
  let page = null;
  let addDialog = null;

  /** @type {?settings.TestSmbBrowserProxy} */
  let smbBrowserProxy = null;

  setup(function() {
    smbBrowserProxy = new TestSmbBrowserProxy();
    settings.SmbBrowserProxyImpl.instance_ = smbBrowserProxy;

    PolymerTest.clearBody();

    page = document.createElement('settings-smb-shares-page');
    document.body.appendChild(page);

    const button = page.$$('#addShare');
    assertTrue(!!button);
    button.click();
    Polymer.dom.flush();

    addDialog = page.$$('settings-add-smb-share-dialog');
    assertTrue(!!addDialog);

    Polymer.dom.flush();
  });

  teardown(function() {
    page.remove();
    addDialog = null;
    page = null;
  });

  test('AddBecomesEnabled', function() {
    const url = addDialog.$.address;

    expectTrue(!!url);
    url.value = 'smb://192.168.1.1/testshare';

    const addButton = addDialog.$$('#actionButton');
    expectTrue(!!addButton);
    expectFalse(addButton.disabled);
  });

  test('ClickAdd', function() {
    const expectedSmbUrl = 'smb://192.168.1.1/testshare';
    const expectedUsername = 'username';
    const expectedPassword = 'password';

    const url = addDialog.$$('#address');
    expectTrue(!!url);
    url.value = expectedSmbUrl;

    const un = addDialog.$$('#username');
    expectTrue(!!un);
    un.value = expectedUsername;

    const pw = addDialog.$$('#password');
    expectTrue(!!pw);
    pw.value = expectedPassword;

    const addButton = addDialog.$$('.action-button');
    expectTrue(!!addButton);

    addButton.click();
    return smbBrowserProxy.whenCalled('smbMount').then(function(args) {
      expectEquals(expectedSmbUrl, args[0]);
      expectEquals(expectedUsername, args[1]);
      expectEquals(expectedPassword, args[2]);
    });
  });
});
