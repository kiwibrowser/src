// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for extension-load-error. */
cr.define('extension_load_error_tests', function() {
  /** @enum {string} */
  var TestNames = {
    RetryError: 'RetryError',
    RetrySuccess: 'RetrySuccess',
    CodeSection: 'Code Section',
  };

  var suiteName = 'ExtensionLoadErrorTests';

  suite(suiteName, function() {
    /** @type {extensions.LoadError} */
    var loadError;

    /** @type {MockDelegate} */
    var mockDelegate;

    var fakeGuid = 'uniqueId';

    var stubLoadError = {
      error: 'error',
      path: 'some/path/',
      retryGuid: fakeGuid,
    };

    setup(function() {
      PolymerTest.clearBody();
      mockDelegate = new extensions.TestService();
      loadError = new extensions.LoadError();
      loadError.delegate = mockDelegate;
      loadError.loadError = stubLoadError;
      document.body.appendChild(loadError);
    });

    test(assert(TestNames.RetryError), function() {
      var dialogElement = loadError.$$('cr-dialog').getNative();
      expectFalse(extension_test_util.isElementVisible(dialogElement));
      loadError.show();
      expectTrue(extension_test_util.isElementVisible(dialogElement));

      mockDelegate.setRetryLoadUnpackedError(stubLoadError);
      MockInteractions.tap(loadError.$$('.action-button'));
      return mockDelegate.whenCalled('retryLoadUnpacked').then(arg => {
        expectEquals(fakeGuid, arg);
        expectTrue(extension_test_util.isElementVisible(dialogElement));
        MockInteractions.tap(loadError.$$('.cancel-button'));
        expectFalse(extension_test_util.isElementVisible(dialogElement));
      });
    });

    test(assert(TestNames.RetrySuccess), function() {
      var dialogElement = loadError.$$('cr-dialog').getNative();
      expectFalse(extension_test_util.isElementVisible(dialogElement));
      loadError.show();
      expectTrue(extension_test_util.isElementVisible(dialogElement));

      MockInteractions.tap(loadError.$$('.action-button'));
      return mockDelegate.whenCalled('retryLoadUnpacked').then(arg => {
        expectEquals(fakeGuid, arg);
        expectFalse(extension_test_util.isElementVisible(dialogElement));
      });
    });

    test(assert(TestNames.CodeSection), function() {
      expectTrue(loadError.$.code.$$('#scroll-container').hidden);
      var loadErrorWithSource = {
        error: 'Some error',
        path: '/some/path',
        source: {
          beforeHighlight: 'before',
          highlight: 'highlight',
          afterHighlight: 'after',
        },
      };

      loadError.loadError = loadErrorWithSource;
      expectFalse(loadError.$.code.$$('#scroll-container').hidden);
    });
  });

  return {
    suiteName: suiteName,
    TestNames: TestNames,
  };
});
