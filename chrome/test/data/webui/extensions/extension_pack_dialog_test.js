// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for extension-pack-dialog. */
cr.define('extension_pack_dialog_tests', function() {
  /** @enum {string} */
  var TestNames = {
    Interaction: 'Interaction',
    PackSuccess: 'PackSuccess',
    PackWarning: 'PackWarning',
    PackError: 'PackError',
  };

  /**
   * @implements {extensions.PackDialogDelegate}
   * @constructor
   */
  function MockDelegate() {
    this.mockResponse = null;
    this.rootPromise;
    this.keyPromise;
  }

  MockDelegate.prototype = {
    /** @override */
    choosePackRootDirectory: function() {
      this.rootPromise = new PromiseResolver();
      return this.rootPromise.promise;
    },

    /** @override */
    choosePrivateKeyPath: function() {
      this.keyPromise = new PromiseResolver();
      return this.keyPromise.promise;
    },

    /** @override */
    packExtension: function(rootPath, keyPath, flag, callback) {
      this.rootPath = rootPath;
      this.keyPath = keyPath;
      this.flag = flag;

      if (callback && this.mockResponse) {
        callback(this.mockResponse);
      }
    },
  };

  var suiteName = 'ExtensionPackDialogTests';

  suite(suiteName, function() {
    /** @type {extensions.PackDialog} */
    var packDialog;

    /** @type {MockDelegate} */
    var mockDelegate;

    setup(function() {
      PolymerTest.clearBody();
      mockDelegate = new MockDelegate();
      packDialog = new extensions.PackDialog();
      packDialog.delegate = mockDelegate;
      document.body.appendChild(packDialog);
    });

    test(assert(TestNames.Interaction), function() {
      var dialogElement = packDialog.$$('cr-dialog').getNative();

      expectFalse(extension_test_util.isElementVisible(dialogElement));
      packDialog.show();
      expectTrue(extension_test_util.isElementVisible(dialogElement));
      expectEquals('', packDialog.$$('#root-dir').value);
      MockInteractions.tap(packDialog.$$('#root-dir-browse'));
      expectTrue(!!mockDelegate.rootPromise);
      expectEquals('', packDialog.$$('#root-dir').value);
      var kRootPath = 'this/is/a/path';

      var promises = [];
      promises.push(mockDelegate.rootPromise.promise.then(function() {
        expectEquals(kRootPath, packDialog.$$('#root-dir').value);
        expectEquals(kRootPath, packDialog.packDirectory_);
      }));

      Polymer.dom.flush();
      expectEquals('', packDialog.$$('#key-file').value);
      MockInteractions.tap(packDialog.$$('#key-file-browse'));
      expectTrue(!!mockDelegate.keyPromise);
      expectEquals('', packDialog.$$('#key-file').value);
      var kKeyPath = 'here/is/another/path';

      promises.push(mockDelegate.keyPromise.promise.then(function() {
        expectEquals(kKeyPath, packDialog.$$('#key-file').value);
        expectEquals(kKeyPath, packDialog.keyFile_);
      }));

      mockDelegate.rootPromise.resolve(kRootPath);
      mockDelegate.keyPromise.resolve(kKeyPath);

      return Promise.all(promises).then(function() {
        MockInteractions.tap(packDialog.$$('.action-button'));
        expectEquals(kRootPath, mockDelegate.rootPath);
        expectEquals(kKeyPath, mockDelegate.keyPath);
      });
    });

    test(assert(TestNames.PackSuccess), function() {
      var dialogElement = packDialog.$$('cr-dialog').getNative();
      var packDialogAlert;
      var alertElement;

      packDialog.show();
      expectTrue(extension_test_util.isElementVisible(dialogElement));

      var kRootPath = 'this/is/a/path';
      mockDelegate.mockResponse = {
        status: chrome.developerPrivate.PackStatus.SUCCESS
      };

      MockInteractions.tap(packDialog.$$('#root-dir-browse'));
      mockDelegate.rootPromise.resolve(kRootPath);

      return mockDelegate.rootPromise.promise
          .then(() => {
            expectEquals(kRootPath, packDialog.$$('#root-dir').value);
            MockInteractions.tap(packDialog.$$('.action-button'));

            return PolymerTest.flushTasks();
          })
          .then(() => {
            packDialogAlert = packDialog.$$('extensions-pack-dialog-alert');
            alertElement = packDialogAlert.$.dialog.getNative();
            expectTrue(extension_test_util.isElementVisible(alertElement));
            expectTrue(extension_test_util.isElementVisible(dialogElement));
            expectTrue(!!packDialogAlert.$$('.action-button'));

            // After 'ok', both dialogs should be closed.
            MockInteractions.tap(packDialogAlert.$$('.action-button'));
            return PolymerTest.flushTasks();
          })
          .then(() => {
            expectFalse(extension_test_util.isElementVisible(alertElement));
            expectFalse(extension_test_util.isElementVisible(dialogElement));
          });
    });

    test(assert(TestNames.PackError), function() {
      var dialogElement = packDialog.$$('cr-dialog').getNative();
      var packDialogAlert;
      var alertElement;

      packDialog.show();
      expectTrue(extension_test_util.isElementVisible(dialogElement));

      var kRootPath = 'this/is/a/path';
      mockDelegate.mockResponse = {
        status: chrome.developerPrivate.PackStatus.ERROR
      };

      MockInteractions.tap(packDialog.$$('#root-dir-browse'));
      mockDelegate.rootPromise.resolve(kRootPath);

      return mockDelegate.rootPromise.promise.then(() => {
        expectEquals(kRootPath, packDialog.$$('#root-dir').value);
        MockInteractions.tap(packDialog.$$('.action-button'));
        Polymer.dom.flush();

        // Make sure new alert and the appropriate buttons are visible.
        packDialogAlert = packDialog.$$('extensions-pack-dialog-alert');
        alertElement = packDialogAlert.$.dialog.getNative();
        expectTrue(extension_test_util.isElementVisible(alertElement));
        expectTrue(extension_test_util.isElementVisible(dialogElement));
        expectTrue(!!packDialogAlert.$$('.action-button'));

        // After cancel, original dialog is still open and values unchanged.
        MockInteractions.tap(packDialogAlert.$$('.action-button'));
        Polymer.dom.flush();
        expectFalse(extension_test_util.isElementVisible(alertElement));
        expectTrue(extension_test_util.isElementVisible(dialogElement));
        expectEquals(kRootPath, packDialog.$$('#root-dir').value);
      });
    });

    test(assert(TestNames.PackWarning), function() {
      var dialogElement = packDialog.$$('cr-dialog').getNative();
      var packDialogAlert;
      var alertElement;

      packDialog.show();
      expectTrue(extension_test_util.isElementVisible(dialogElement));

      var kRootPath = 'this/is/a/path';
      mockDelegate.mockResponse = {
        status: chrome.developerPrivate.PackStatus.WARNING,
        item_path: 'item_path',
        pem_path: 'pem_path',
        override_flags: 1,
      };

      MockInteractions.tap(packDialog.$$('#root-dir-browse'));
      mockDelegate.rootPromise.resolve(kRootPath);

      return mockDelegate.rootPromise.promise
          .then(() => {
            expectEquals(kRootPath, packDialog.$$('#root-dir').value);
            MockInteractions.tap(packDialog.$$('.action-button'));
            Polymer.dom.flush();

            // Make sure new alert and the appropriate buttons are visible.
            packDialogAlert = packDialog.$$('extensions-pack-dialog-alert');
            alertElement = packDialogAlert.$.dialog.getNative();
            expectTrue(extension_test_util.isElementVisible(alertElement));
            expectTrue(extension_test_util.isElementVisible(dialogElement));
            expectFalse(packDialogAlert.$$('.cancel-button').hidden);
            expectFalse(packDialogAlert.$$('.action-button').hidden);

            // Make sure "proceed anyway" try to pack extension again.
            MockInteractions.tap(packDialogAlert.$$('.action-button'));

            return PolymerTest.flushTasks();
          })
          .then(() => {
            // Make sure packExtension is called again with the right params.
            expectFalse(extension_test_util.isElementVisible(alertElement));
            expectEquals(
                mockDelegate.flag, mockDelegate.mockResponse.override_flags);
          });
    });
  });

  return {
    suiteName: suiteName,
    TestNames: TestNames,
  };
});
