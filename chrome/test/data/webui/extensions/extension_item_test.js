// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for extension-item. */
cr.define('extension_item_tests', function() {
  /**
   * The data used to populate the extension item.
   * @type {chrome.developerPrivate.ExtensionInfo}
   */
  var extensionData = extension_test_util.createExtensionInfo();

  // The normal elements, which should always be shown.
  var normalElements = [
    {selector: '#name', text: extensionData.name},
    {selector: '#icon'},
    {selector: '#description', text: extensionData.description},
    {selector: '#enable-toggle'},
    {selector: '#details-button'},
    {selector: '#remove-button'},
  ];
  // The developer elements, which should only be shown if in developer
  // mode *and* showing details.
  var devElements = [
    {selector: '#version', text: extensionData.version},
    {selector: '#extension-id', text: `ID: ${extensionData.id}`},
    {selector: '#inspect-views'},
    {selector: '#inspect-views a[is="action-link"]', text: 'foo.html,'},
    {
      selector: '#inspect-views a[is="action-link"]:nth-of-type(2)',
      text: '1 more…'
    },
  ];

  /**
   * Tests that the elements' visibility matches the expected visibility.
   * @param {extensions.Item} item
   * @param {Array<Object<string>>} elements
   * @param {boolean} visibility
   */
  function testElementsVisibility(item, elements, visibility) {
    elements.forEach(function(element) {
      extension_test_util.testVisible(
          item, element.selector, visibility, element.text);
    });
  }

  /** Tests that normal elements are visible. */
  function testNormalElementsAreVisible(item) {
    testElementsVisibility(item, normalElements, true);
  }

  /** Tests that normal elements are hidden. */
  function testNormalElementsAreHidden(item) {
    testElementsVisibility(item, normalElements, false);
  }

  /** Tests that dev elements are visible. */
  function testDeveloperElementsAreVisible(item) {
    testElementsVisibility(item, devElements, true);
  }

  /** Tests that dev elements are hidden. */
  function testDeveloperElementsAreHidden(item) {
    testElementsVisibility(item, devElements, false);
  }

  /** @enum {string} */
  var TestNames = {
    ElementVisibilityNormalState: 'element visibility: normal state',
    ElementVisibilityDeveloperState:
        'element visibility: after enabling developer mode',
    ClickableItems: 'clickable items',
    FailedReloadFiresLoadError: 'failed reload fires load error',
    Warnings: 'warnings',
    SourceIndicator: 'source indicator',
    EnableToggle: 'toggle is disabled when necessary',
    RemoveButton: 'remove button hidden when necessary',
    HtmlInName: 'html in extension name',
  };

  var suiteName = 'ExtensionItemTest';

  suite(suiteName, function() {
    /**
     * Extension item created before each test.
     * @type {extensions.Item}
     */
    var item;

    /** @type {extension_test_util.MockItemDelegate} */
    var mockDelegate;

    // Initialize an extension item before each test.
    setup(function() {
      PolymerTest.clearBody();
      mockDelegate = new extension_test_util.MockItemDelegate();
      item = new extensions.Item();
      item.set('data', extensionData);
      item.set('delegate', mockDelegate);
      document.body.appendChild(item);
    });

    test(assert(TestNames.ElementVisibilityNormalState), function() {
      testNormalElementsAreVisible(item);
      testDeveloperElementsAreHidden(item);

      expectTrue(item.$['enable-toggle'].checked);
      item.set('data.state', 'DISABLED');
      expectFalse(item.$['enable-toggle'].checked);
      item.set('data.state', 'BLACKLISTED');
      expectFalse(item.$['enable-toggle'].checked);
    });

    test(assert(TestNames.ElementVisibilityDeveloperState), function() {
      item.set('inDevMode', true);

      testNormalElementsAreVisible(item);
      testDeveloperElementsAreVisible(item);

      // Developer reload button should be visible only for enabled unpacked
      // extensions.
      extension_test_util.testVisible(item, '#dev-reload-button', false);

      item.set('data.location', chrome.developerPrivate.Location.UNPACKED);
      Polymer.dom.flush();
      extension_test_util.testVisible(item, '#dev-reload-button', true);

      item.set('data.state', chrome.developerPrivate.ExtensionState.DISABLED);
      Polymer.dom.flush();
      extension_test_util.testVisible(item, '#dev-reload-button', false);

      item.set('data.state', chrome.developerPrivate.ExtensionState.TERMINATED);
      Polymer.dom.flush();
      extension_test_util.testVisible(item, '#dev-reload-button', false);
    });

    /** Tests that the delegate methods are correctly called. */
    test(assert(TestNames.ClickableItems), function() {
      item.set('inDevMode', true);

      mockDelegate.testClickingCalls(
          item.$['remove-button'], 'deleteItem', [item.data.id]);
      mockDelegate.testClickingCalls(
          item.$['enable-toggle'], 'setItemEnabled', [item.data.id, false]);
      mockDelegate.testClickingCalls(
          item.$$('#inspect-views a[is="action-link"]'), 'inspectItemView',
          [item.data.id, item.data.views[0]]);

      // Setup for testing navigation buttons.
      var currentPage = null;
      extensions.navigation.addListener(newPage => {
        currentPage = newPage;
      });

      MockInteractions.tap(item.$$('#details-button'));
      expectDeepEquals(
          currentPage, {page: Page.DETAILS, extensionId: item.data.id});

      // Reset current page and test inspect-view navigation.
      extensions.navigation.navigateTo({page: Page.LIST});
      currentPage = null;
      MockInteractions.tap(
          item.$$('#inspect-views a[is="action-link"]:nth-of-type(2)'));
      expectDeepEquals(
          currentPage, {page: Page.DETAILS, extensionId: item.data.id});

      item.set('data.disableReasons.corruptInstall', true);
      Polymer.dom.flush();
      mockDelegate.testClickingCalls(
          item.$$('#repair-button'), 'repairItem', [item.data.id]);

      item.set('data.state', chrome.developerPrivate.ExtensionState.TERMINATED);
      Polymer.dom.flush();
      mockDelegate.testClickingCalls(
          item.$$('#terminated-reload-button'), 'reloadItem', [item.data.id],
          Promise.resolve());

      item.set('data.location', chrome.developerPrivate.Location.UNPACKED);
      item.set('data.state', chrome.developerPrivate.ExtensionState.ENABLED);
      Polymer.dom.flush();
    });

    /** Tests that the reload button properly fires the load-error event. */
    test(assert(TestNames.FailedReloadFiresLoadError), function() {
      item.set('inDevMode', true);
      item.set('data.location', chrome.developerPrivate.Location.UNPACKED);
      Polymer.dom.flush();
      extension_test_util.testVisible(item, '#dev-reload-button', true);

      // Check clicking the reload button. The reload button should fire a
      // load-error event if and only if the reload fails (indicated by a
      // rejected promise).
      // This is a bit of a pain to verify because the promises finish
      // asynchronously, so we have to use setTimeout()s.
      var firedLoadError = false;
      item.addEventListener('load-error', () => {
        firedLoadError = true;
      });

      // This is easier to test with a TestBrowserProxy-style delegate.
      var proxyDelegate = new extensions.TestService();
      item.delegate = proxyDelegate;

      var verifyEventPromise = function(expectCalled) {
        return new Promise((resolve, reject) => {
          setTimeout(() => {
            expectEquals(expectCalled, firedLoadError);
            resolve();
          });
        });
      };

      MockInteractions.tap(item.$$('#dev-reload-button'));
      return proxyDelegate.whenCalled('reloadItem')
          .then(function(id) {
            expectEquals(item.data.id, id);
            return verifyEventPromise(false);
          })
          .then(function() {
            proxyDelegate.resetResolver('reloadItem');
            proxyDelegate.setForceReloadItemError(true);
            MockInteractions.tap(item.$$('#dev-reload-button'));
            return proxyDelegate.whenCalled('reloadItem');
          })
          .then(function(id) {
            expectEquals(item.data.id, id);
            return verifyEventPromise(true);
          });
    });

    test(assert(TestNames.Warnings), function() {
      const kCorrupt = 1 << 0;
      const kSuspicious = 1 << 1;
      const kBlacklisted = 1 << 2;
      const kRuntime = 1 << 3;

      function assertWarnings(mask) {
        const isVisible = extension_test_util.isVisible;
        assertEquals(
            !!(mask & kCorrupt), isVisible(item, '#corrupted-warning'));
        assertEquals(
            !!(mask & kSuspicious), isVisible(item, '#suspicious-warning'));
        assertEquals(
            !!(mask & kBlacklisted), isVisible(item, '#blacklisted-warning'));
        assertEquals(!!(mask & kRuntime), isVisible(item, '#runtime-warnings'));
      }

      assertWarnings(0);

      item.set('data.disableReasons.corruptInstall', true);
      Polymer.dom.flush();
      assertWarnings(kCorrupt);

      item.set('data.disableReasons.suspiciousInstall', true);
      Polymer.dom.flush();
      assertWarnings(kCorrupt | kSuspicious);

      item.set('data.blacklistText', 'This item is blacklisted');
      Polymer.dom.flush();
      assertWarnings(kCorrupt | kSuspicious | kBlacklisted);

      item.set('data.blacklistText', undefined);
      Polymer.dom.flush();
      assertWarnings(kCorrupt | kSuspicious);

      item.set('data.runtimeWarnings', ['Dummy warning']);
      Polymer.dom.flush();
      assertWarnings(kCorrupt | kSuspicious | kRuntime);

      item.set('data.disableReasons.corruptInstall', false);
      item.set('data.disableReasons.suspiciousInstall', false);
      item.set('data.runtimeWarnings', []);
      Polymer.dom.flush();
      assertWarnings(0);
    });

    test(assert(TestNames.SourceIndicator), function() {
      expectFalse(extension_test_util.isVisible(item, '#source-indicator'));
      item.set('data.location', 'UNPACKED');
      Polymer.dom.flush();
      expectTrue(extension_test_util.isVisible(item, '#source-indicator'));
      var icon = item.$$('#source-indicator iron-icon');
      assertTrue(!!icon);
      expectEquals('extensions-icons:unpacked', icon.icon);
      extension_test_util.testIcons(item);

      item.set('data.location', 'THIRD_PARTY');
      Polymer.dom.flush();
      expectTrue(extension_test_util.isVisible(item, '#source-indicator'));
      expectEquals('input', icon.icon);
      extension_test_util.testIcons(item);

      item.set('data.location', 'UNKNOWN');
      Polymer.dom.flush();
      expectTrue(extension_test_util.isVisible(item, '#source-indicator'));
      expectEquals('input', icon.icon);
      extension_test_util.testIcons(item);

      item.set('data.location', 'FROM_STORE');
      item.set('data.controlledInfo', {type: 'POLICY', text: 'policy'});
      Polymer.dom.flush();
      expectTrue(extension_test_util.isVisible(item, '#source-indicator'));
      expectEquals('communication:business', icon.icon);
      extension_test_util.testIcons(item);

      item.set('data.controlledInfo', null);
      Polymer.dom.flush();
      expectFalse(extension_test_util.isVisible(item, '#source-indicator'));
    });

    test(assert(TestNames.EnableToggle), function() {
      expectFalse(item.$['enable-toggle'].disabled);

      // Test case where user does not have permission.
      item.set('data.userMayModify', false);
      Polymer.dom.flush();
      expectTrue(item.$['enable-toggle'].disabled);

      // Test case of a blacklisted extension.
      item.set('data.userMayModify', true);
      item.set('data.state', 'BLACKLISTED');
      Polymer.dom.flush();
      expectTrue(item.$['enable-toggle'].disabled);
    });

    test(assert(TestNames.RemoveButton), function() {
      expectFalse(item.$['remove-button'].hidden);
      item.set('data.controlledInfo', {type: 'POLICY', text: 'policy'});
      Polymer.dom.flush();
      expectTrue(item.$['remove-button'].hidden);
    });

    test(assert(TestNames.HtmlInName), function() {
      let name = '<HTML> in the name!';
      item.set('data.name', name);
      Polymer.dom.flush();
      assertEquals(name, item.$.name.textContent.trim());
      // "Related to $1" is IDS_MD_EXTENSIONS_EXTENSION_A11Y_ASSOCIATION.
      assertEquals(
          `Related to ${name}`, item.$.a11yAssociation.textContent.trim());
    });
  });

  return {
    suiteName: suiteName,
    TestNames: TestNames,
  };
});
