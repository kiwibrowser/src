// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for extensions-detail-view. */
cr.define('extension_error_page_tests', function() {
  /** @enum {string} */
  var TestNames = {
    Layout: 'layout',
    CodeSection: 'code section',
    ErrorSelection: 'error selection',
  };

  /**
   * @constructor
   * @extends {extension_test_util.ClickMock}
   * @implements {extensions.ErrorPageDelegate}
   */
  function MockErrorPageDelegate() {}

  MockErrorPageDelegate.prototype = {
    __proto__: extension_test_util.ClickMock.prototype,

    /** @override */
    deleteErrors: function(extensionId, errorIds, type) {},

    /** @override */
    requestFileSource: function(args) {
      this.requestFileSourceArgs = args;
      this.requestFileSourceResolver = new PromiseResolver();
      return this.requestFileSourceResolver.promise;
    },
  };

  var suiteName = 'ExtensionErrorPageTest';

  suite(suiteName, function() {
    /** @type {chrome.developerPrivate.ExtensionInfo} */
    var extensionData;

    /** @type {extensions.ErrorPage} */
    var errorPage;

    /** @type {MockErrorPageDelegate} */
    var mockDelegate;

    var extensionId = 'a'.repeat(32);

    // Common data for runtime errors.
    var runtimeErrorBase = {
      type: chrome.developerPrivate.ErrorType.RUNTIME,
      extensionId: extensionId,
      fromIncognito: false,
    };

    // Common data for manifest errors.
    var manifestErrorBase = {
      type: chrome.developerPrivate.ErrorType.MANIFEST,
      extensionId: extensionId,
      fromIncognito: false,
    };

    // Initialize an extension item before each test.
    setup(function() {
      PolymerTest.clearBody();
      var runtimeError = Object.assign(
          {
            source: 'chrome-extension://' + extensionId + '/source.html',
            message: 'message',
            id: 1,
            severity: chrome.developerPrivate.ErrorLevel.ERROR,
          },
          runtimeErrorBase);
      extensionData = extension_test_util.createExtensionInfo({
        runtimeErrors: [runtimeError],
        manifestErrors: [],
      });
      errorPage = new extensions.ErrorPage();
      mockDelegate = new MockErrorPageDelegate();
      errorPage.delegate = mockDelegate;
      errorPage.data = extensionData;
      document.body.appendChild(errorPage);
    });

    test(assert(TestNames.Layout), function() {
      Polymer.dom.flush();

      extension_test_util.testIcons(errorPage);

      var testIsVisible = extension_test_util.isVisible.bind(null, errorPage);
      expectTrue(testIsVisible('#closeButton'));
      expectTrue(testIsVisible('#heading'));
      expectTrue(testIsVisible('#errorsList'));

      var errorElements = errorPage.querySelectorAll('* /deep/ .error-item');
      expectEquals(1, errorElements.length);
      var error = errorElements[0];
      expectEquals(
          'message', error.querySelector('.error-message').textContent.trim());
      expectTrue(error.querySelector('iron-icon').icon == 'error');

      var manifestError = Object.assign(
          {
            source: 'manifest.json',
            message: 'invalid key',
            id: 2,
            manifestKey: 'permissions',
          },
          manifestErrorBase);
      errorPage.set('data.manifestErrors', [manifestError]);
      Polymer.dom.flush();
      errorElements = errorPage.querySelectorAll('* /deep/ .error-item');
      expectEquals(2, errorElements.length);
      error = errorElements[0];
      expectEquals(
          'invalid key',
          error.querySelector('.error-message').textContent.trim());
      expectTrue(error.querySelector('iron-icon').icon == 'warning');

      mockDelegate.testClickingCalls(
          error.querySelector('.icon-delete-gray button'), 'deleteErrors',
          [extensionId, [manifestError.id]]);
    });

    test(assert(TestNames.CodeSection), function(done) {
      Polymer.dom.flush();

      expectTrue(!!mockDelegate.requestFileSourceArgs);
      args = mockDelegate.requestFileSourceArgs;
      expectEquals(extensionId, args.extensionId);
      expectEquals('source.html', args.pathSuffix);
      expectEquals('message', args.message);

      expectTrue(!!mockDelegate.requestFileSourceResolver);
      var code = {
        beforeHighlight: 'foo',
        highlight: 'bar',
        afterHighlight: 'baz',
        message: 'quu',
      };
      mockDelegate.requestFileSourceResolver.resolve(code);
      mockDelegate.requestFileSourceResolver.promise.then(function() {
        Polymer.dom.flush();
        expectEquals(code, errorPage.$$('extensions-code-section').code);
        done();
      });
    });

    test(assert(TestNames.ErrorSelection), function() {
      var nextRuntimeError = Object.assign(
          {
            source: 'chrome-extension://' + extensionId + '/other_source.html',
            message: 'Other error',
            id: 2,
            severity: chrome.developerPrivate.ErrorLevel.ERROR,
            renderProcessId: 111,
            renderViewId: 222,
            canInspect: true,
            contextUrl: 'http://test.com',
            stackTrace: [{url: 'url', lineNumber: 123, columnNumber: 321}],
          },
          runtimeErrorBase);
      // Add a new runtime error to the end.
      errorPage.push('data.runtimeErrors', nextRuntimeError);
      Polymer.dom.flush();

      var errorElements =
          errorPage.querySelectorAll('* /deep/ .error-item .start');
      var ironCollapses = errorPage.querySelectorAll('* /deep/ iron-collapse');
      expectEquals(2, errorElements.length);
      expectEquals(2, ironCollapses.length);

      // The first error should be focused by default, and we should have
      // requested the source for it.
      expectEquals(
          extensionData.runtimeErrors[0], errorPage.getSelectedError());
      expectTrue(!!mockDelegate.requestFileSourceArgs);
      var args = mockDelegate.requestFileSourceArgs;
      expectEquals('source.html', args.pathSuffix);
      expectTrue(ironCollapses[0].opened);
      expectFalse(ironCollapses[1].opened);
      mockDelegate.requestFileSourceResolver.resolve(null);

      mockDelegate.requestFileSourceResolver = new PromiseResolver();
      mockDelegate.requestFileSourceArgs = undefined;

      // Tap the second error. It should now be selected and we should request
      // the source for it.
      MockInteractions.tap(errorElements[1]);
      expectEquals(nextRuntimeError, errorPage.getSelectedError());
      expectTrue(!!mockDelegate.requestFileSourceArgs);
      args = mockDelegate.requestFileSourceArgs;
      expectEquals('other_source.html', args.pathSuffix);
      expectTrue(ironCollapses[1].opened);
      expectFalse(ironCollapses[0].opened);

      expectEquals(
          'Unknown',
          ironCollapses[0].querySelector('.context-url').textContent.trim());
      expectEquals(
          nextRuntimeError.contextUrl,
          ironCollapses[1].querySelector('.context-url').textContent.trim());
    });
  });

  return {
    suiteName: suiteName,
    TestNames: TestNames,
  };
});
