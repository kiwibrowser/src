// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('extension_navigation_helper_tests', function() {
  /** @enum {string} */
  var TestNames = {
    Basic: 'basic',
    Conversions: 'conversions',
    PushAndReplaceState: 'push and replace state',
    SupportedRoutes: 'supported routes'
  };

  /**
   * @return {!Promise<void>} A promise that resolves after the next popstate
   *     event.
   */
  function getOnPopState() {
    return new Promise(function(resolve, reject) {
      window.addEventListener('popstate', function listener() {
        window.removeEventListener('popstate', listener);
        // Resolve asynchronously to allow all other listeners to run.
        window.setTimeout(resolve, 0);
      });
    });
  }

  var suiteName = 'ExtensionNavigationHelperTest';

  suite(suiteName, function() {
    let navigationHelper;

    setup(function() {
      PolymerTest.clearBody();
      Polymer.dom.flush();
      navigationHelper = new extensions.NavigationHelper();
    });

    test(assert(TestNames.Basic), function() {
      var id = 'a'.repeat(32);
      var mock = new MockMethod();
      var changePage = function(state) {
        mock.recordCall([state]);
      };

      navigationHelper.addListener(changePage);

      expectDeepEquals({page: Page.LIST}, navigationHelper.getCurrentPage());

      var currentLength = history.length;
      navigationHelper.updateHistory({page: Page.DETAILS, extensionId: id});
      expectEquals(++currentLength, history.length);

      navigationHelper.updateHistory({page: Page.ERRORS, extensionId: id});
      expectEquals(++currentLength, history.length);

      mock.addExpectation({page: Page.DETAILS, extensionId: id});
      var waitForPop = getOnPopState();
      history.back();
      return waitForPop
          .then(() => {
            mock.verifyMock();

            mock.addExpectation({page: Page.LIST});
            var waitForNextPop = getOnPopState();
            history.back();
            return waitForNextPop;
          })
          .then(() => {
            mock.verifyMock();
          });
    });

    test(assert(TestNames.Conversions), function() {
      var id = 'a'.repeat(32);
      var stateUrlPairs = {
        extensions: {
          url: 'chrome://extensions/',
          state: {page: Page.LIST},
        },
        details: {
          url: 'chrome://extensions/?id=' + id,
          state: {page: Page.DETAILS, extensionId: id},
        },
        options: {
          url: 'chrome://extensions/?options=' + id,
          state: {
            page: Page.DETAILS,
            extensionId: id,
            subpage: Dialog.OPTIONS,
          },
        },
        errors: {
          url: 'chrome://extensions/?errors=' + id,
          state: {page: Page.ERRORS, extensionId: id},
        },
        shortcuts: {
          url: 'chrome://extensions/shortcuts',
          state: {page: Page.SHORTCUTS},
        },
      };

      // Test url -> state.
      for (let key in stateUrlPairs) {
        let entry = stateUrlPairs[key];
        history.pushState({}, '', entry.url);
        expectDeepEquals(entry.state, navigationHelper.getCurrentPage(), key);
      }

      // Test state -> url.
      for (let key in stateUrlPairs) {
        let entry = stateUrlPairs[key];
        navigationHelper.updateHistory(entry.state);
        expectEquals(entry.url, location.href, key);
      }
    });

    test(assert(TestNames.PushAndReplaceState), function() {
      var id1 = 'a'.repeat(32);
      var id2 = 'b'.repeat(32);

      history.pushState({}, '', 'chrome://extensions/');
      expectDeepEquals({page: Page.LIST}, navigationHelper.getCurrentPage());

      var expectedLength = history.length;

      // Navigating to a new page pushes new state.
      navigationHelper.updateHistory({page: Page.DETAILS, extensionId: id1});
      expectEquals(++expectedLength, history.length);

      // Navigating to a subpage (like the options page) just opens a dialog,
      // and shouldn't push new state.
      navigationHelper.updateHistory(
          {page: Page.DETAILS, extensionId: id1, subpage: Dialog.OPTIONS});
      expectEquals(expectedLength, history.length);

      // Navigating away from a subpage also shouldn't push state (it just
      // closes the dialog).
      navigationHelper.updateHistory({page: Page.DETAILS, extensionId: id1});
      expectEquals(expectedLength, history.length);

      // Navigating away should push new state.
      navigationHelper.updateHistory({page: Page.LIST});
      expectEquals(++expectedLength, history.length);

      // Navigating to a subpage of a different page should push state.
      navigationHelper.updateHistory(
          {page: Page.DETAILS, extensionId: id1, subpage: Dialog.OPTIONS});
      expectEquals(++expectedLength, history.length);

      // Navigating away from a subpage to a page for a different item should
      // push state.
      navigationHelper.updateHistory({page: Page.DETAILS, extensionId: id2});
      expectEquals(++expectedLength, history.length);

      // Using replaceWith, which passes true for replaceState should not push
      // state.
      navigationHelper.updateHistory(
          {page: Page.DETAILS, extensionId: id1}, true /* replaceState */);
      expectEquals(expectedLength, history.length);
    });

    test(assert(TestNames.SupportedRoutes), function() {
      function removeEndSlash(url) {
        const CANONICAL_PATH_REGEX = /([\/-\w]+)\/$/;
        return url.replace(CANONICAL_PATH_REGEX, '$1');
      }

      // If it should not redirect, leave newUrl as undefined.
      function testIfRedirected(url, newUrl) {
        history.pushState({}, '', url);
        const testNavigationHelper = new extensions.NavigationHelper();
        expectEquals(
            removeEndSlash(window.location.href),
            removeEndSlash(newUrl || url));
      }

      testIfRedirected('chrome://extensions');
      testIfRedirected('chrome://extensions/');
      testIfRedirected('chrome://extensions/shortcuts');
      testIfRedirected('chrome://extensions/shortcuts/');
      testIfRedirected('chrome://extensions/fake-route', 'chrome://extensions');
      // Test trailing slash works.

      // Test legacy paths
      testIfRedirected(
          'chrome://extensions/configureCommands',
          'chrome://extensions/shortcuts');
    });
  });

  return {
    suiteName: suiteName,
    TestNames: TestNames,
  };
});
