// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for extensions-detail-view. */
cr.define('extension_detail_view_tests', function() {
  /** @enum {string} */
  var TestNames = {
    Layout: 'layout',
    LayoutSource: 'layout of source section',
    ClickableElements: 'clickable elements',
    Indicator: 'indicator',
    Warnings: 'warnings',
  };

  var suiteName = 'ExtensionDetailViewTest';

  suite(suiteName, function() {
    /**
     * Extension item created before each test.
     * @type {extensions.Item}
     */
    var item;

    /**
     * Backing extension data for the item.
     * @type {chrome.developerPrivate.ExtensionInfo}
     */
    var extensionData;

    /** @type {extension_test_util.MockItemDelegate} */
    var mockDelegate;

    // Initialize an extension item before each test.
    setup(function() {
      PolymerTest.clearBody();
      extensionData = extension_test_util.createExtensionInfo({
        incognitoAccess: {isEnabled: true, isActive: false},
        fileAccess: {isEnabled: true, isActive: false},
        runOnAllUrls: {isEnabled: true, isActive: false},
        errorCollection: {isEnabled: true, isActive: false},
      });
      mockDelegate = new extension_test_util.MockItemDelegate();
      item = new extensions.DetailView();
      item.set('data', extensionData);
      item.set('delegate', mockDelegate);
      item.set('inDevMode', false);
      item.set('incognitoAvailable', true);
      document.body.appendChild(item);
    });

    test(assert(TestNames.Layout), function() {
      Polymer.dom.flush();

      extension_test_util.testIcons(item);

      var testIsVisible = extension_test_util.isVisible.bind(null, item);
      expectTrue(testIsVisible('#closeButton'));
      expectTrue(testIsVisible('#icon'));
      expectTrue(testIsVisible('#enable-toggle'));
      expectFalse(testIsVisible('#extensions-options'));
      expectTrue(
          item.$.description.textContent.indexOf('This is an extension') !==
          -1);

      // Check the checkboxes visibility and state. They should be visible
      // only if the associated option is enabled, and checked if the
      // associated option is active.
      var accessOptions = [
        {key: 'incognitoAccess', id: '#allow-incognito'},
        {key: 'fileAccess', id: '#allow-on-file-urls'},
        {key: 'runOnAllUrls', id: '#allow-on-all-sites'},
        {key: 'errorCollection', id: '#collect-errors'},
      ];
      var isChecked = id => item.$$(id).checked;
      for (let option of accessOptions) {
        expectTrue(extension_test_util.isVisible(item, option.id));
        expectFalse(isChecked(option.id), option.id);
        item.set('data.' + option.key + '.isEnabled', false);
        Polymer.dom.flush();
        expectFalse(extension_test_util.isVisible(item, option.id));
        item.set('data.' + option.key + '.isEnabled', true);
        item.set('data.' + option.key + '.isActive', true);
        Polymer.dom.flush();
        expectTrue(extension_test_util.isVisible(item, option.id));
        expectTrue(isChecked(option.id));
      }

      expectFalse(testIsVisible('#dependent-extensions-list'));
      item.set(
          'data.dependentExtensions',
          [{id: 'aaa', name: 'Dependent1'}, {id: 'bbb', name: 'Dependent2'}]);
      Polymer.dom.flush();
      expectTrue(testIsVisible('#dependent-extensions-list'));
      expectEquals(
          2,
          item.$$('#dependent-extensions-list').querySelectorAll('li').length);

      expectFalse(testIsVisible('#permissions-list'));
      expectTrue(testIsVisible('#no-permissions'));
      item.set('data.permissions', ['Permission 1', 'Permission 2']);
      Polymer.dom.flush();
      expectTrue(testIsVisible('#permissions-list'));
      expectEquals(
          2, item.$$('#permissions-list').querySelectorAll('li').length);
      expectFalse(testIsVisible('#no-permissions'));

      var optionsUrl =
          'chrome-extension://' + extensionData.id + '/options.html';
      item.set('data.optionsPage', {openInTab: true, url: optionsUrl});
      expectTrue(testIsVisible('#extensions-options'));

      item.set('data.manifestHomePageUrl', 'http://example.com');
      Polymer.dom.flush();
      expectTrue(testIsVisible('#extensionWebsite'));
      item.set('data.manifestHomePageUrl', '');
      Polymer.dom.flush();
      expectFalse(testIsVisible('#extensionWebsite'));

      item.set('data.webStoreUrl', 'http://example.com');
      Polymer.dom.flush();
      expectTrue(testIsVisible('#viewInStore'));
      item.set('data.webStoreUrl', '');
      Polymer.dom.flush();
      expectFalse(testIsVisible('#viewInStore'));

      expectFalse(testIsVisible('#id-section'));
      expectFalse(testIsVisible('#inspectable-views'));

      item.set('inDevMode', true);
      Polymer.dom.flush();
      expectTrue(testIsVisible('#id-section'));
      expectTrue(testIsVisible('#inspectable-views'));

      assertTrue(item.data.incognitoAccess.isEnabled);
      item.set('incognitoAvailable', false);
      Polymer.dom.flush();
      expectFalse(testIsVisible('#allow-incognito'));

      item.set('incognitoAvailable', true);
      Polymer.dom.flush();
      expectTrue(testIsVisible('#allow-incognito'));

      // Ensure that the "Extension options" button is disabled when the item
      // itself is disabled.
      var extensionOptions = item.$$('#extensions-options');
      assertFalse(extensionOptions.disabled);
      item.set('data.state', chrome.developerPrivate.ExtensionState.DISABLED);
      Polymer.dom.flush();
      assertTrue(extensionOptions.disabled);

      expectFalse(testIsVisible('.warning-icon'));
      item.set('data.runtimeWarnings', ['Dummy warning']);
      Polymer.dom.flush();
      expectTrue(testIsVisible('.warning-icon'));
    });

    test(assert(TestNames.LayoutSource), function() {
      item.set('data.location', 'FROM_STORE');
      Polymer.dom.flush();
      assertEquals('Chrome Web Store', item.$.source.textContent.trim());
      assertFalse(extension_test_util.isVisible(item, '#load-path'));

      item.set('data.location', 'THIRD_PARTY');
      Polymer.dom.flush();
      assertEquals('Added by a third-party', item.$.source.textContent.trim());
      assertFalse(extension_test_util.isVisible(item, '#load-path'));

      item.set('data.location', 'UNPACKED');
      item.set('data.prettifiedPath', 'foo/bar/baz/');
      Polymer.dom.flush();
      assertEquals('Unpacked extension', item.$.source.textContent.trim());
      // Test whether the load path is displayed for unpacked extensions.
      assertTrue(extension_test_util.isVisible(item, '#load-path'));

      item.set('data.location', 'UNKNOWN');
      item.set('data.prettifiedPath', '');
      // |locationText| is expected to always be set if location is UNKNOWN.
      item.set('data.locationText', 'Foo');
      Polymer.dom.flush();
      assertEquals('Foo', item.$.source.textContent.trim());
      assertFalse(extension_test_util.isVisible(item, '#load-path'));
    });

    test(assert(TestNames.ClickableElements), function() {
      var optionsUrl =
          'chrome-extension://' + extensionData.id + '/options.html';
      item.set('data.optionsPage', {openInTab: true, url: optionsUrl});
      item.set('data.prettifiedPath', 'foo/bar/baz/');
      Polymer.dom.flush();

      mockDelegate.testClickingCalls(
          item.$$('#allow-incognito').getLabel(), 'setItemAllowedIncognito',
          [extensionData.id, true]);
      mockDelegate.testClickingCalls(
          item.$$('#allow-on-file-urls').getLabel(), 'setItemAllowedOnFileUrls',
          [extensionData.id, true]);
      mockDelegate.testClickingCalls(
          item.$$('#allow-on-all-sites').getLabel(), 'setItemAllowedOnAllSites',
          [extensionData.id, true]);
      mockDelegate.testClickingCalls(
          item.$$('#collect-errors').getLabel(), 'setItemCollectsErrors',
          [extensionData.id, true]);
      mockDelegate.testClickingCalls(
          item.$$('#extensions-options'), 'showItemOptionsPage',
          [extensionData]);
      mockDelegate.testClickingCalls(
          item.$$('#remove-extension'), 'deleteItem', [extensionData.id]);
      mockDelegate.testClickingCalls(
          item.$$('#load-path > a[is=\'action-link\']'), 'showInFolder',
          [extensionData.id]);
      mockDelegate.testClickingCalls(
          item.$$('#reload-button'), 'reloadItem', [extensionData.id],
          Promise.resolve());
    });

    test(assert(TestNames.Indicator), function() {
      var indicator = item.$$('cr-tooltip-icon');
      expectTrue(indicator.hidden);
      item.set('data.controlledInfo', {type: 'POLICY', text: 'policy'});
      Polymer.dom.flush();
      expectFalse(indicator.hidden);
    });

    test(assert(TestNames.Warnings), function() {
      var testWarningVisible = function(id, isVisible) {
        var f = isVisible ? expectTrue : expectFalse;
        f(extension_test_util.isVisible(item, id));
      };

      testWarningVisible('#runtime-warnings', false);
      testWarningVisible('#corrupted-warning', false);
      testWarningVisible('#suspicious-warning', false);
      testWarningVisible('#blacklisted-warning', false);
      testWarningVisible('#update-required-warning', false);

      item.set('data.runtimeWarnings', ['Dummy warning']);
      Polymer.dom.flush();
      testWarningVisible('#runtime-warnings', true);
      testWarningVisible('#corrupted-warning', false);
      testWarningVisible('#suspicious-warning', false);
      testWarningVisible('#blacklisted-warning', false);
      testWarningVisible('#update-required-warning', false);

      item.set('data.disableReasons.corruptInstall', true);
      Polymer.dom.flush();
      testWarningVisible('#runtime-warnings', true);
      testWarningVisible('#corrupted-warning', true);
      testWarningVisible('#suspicious-warning', false);
      testWarningVisible('#blacklisted-warning', false);
      testWarningVisible('#update-required-warning', false);

      item.set('data.disableReasons.suspiciousInstall', true);
      Polymer.dom.flush();
      testWarningVisible('#runtime-warnings', true);
      testWarningVisible('#corrupted-warning', true);
      testWarningVisible('#suspicious-warning', true);
      testWarningVisible('#blacklisted-warning', false);
      testWarningVisible('#update-required-warning', false);

      item.set('data.blacklistText', 'This item is blacklisted');
      Polymer.dom.flush();
      testWarningVisible('#runtime-warnings', true);
      testWarningVisible('#corrupted-warning', true);
      testWarningVisible('#suspicious-warning', true);
      testWarningVisible('#blacklisted-warning', true);
      testWarningVisible('#update-required-warning', false);

      item.set('data.blacklistText', undefined);
      Polymer.dom.flush();
      testWarningVisible('#runtime-warnings', true);
      testWarningVisible('#corrupted-warning', true);
      testWarningVisible('#suspicious-warning', true);
      testWarningVisible('#blacklisted-warning', false);
      testWarningVisible('#update-required-warning', false);

      item.set('data.disableReasons.updateRequired', true);
      Polymer.dom.flush();
      testWarningVisible('#runtime-warnings', true);
      testWarningVisible('#corrupted-warning', true);
      testWarningVisible('#suspicious-warning', true);
      testWarningVisible('#blacklisted-warning', false);
      testWarningVisible('#update-required-warning', true);

      item.set('data.runtimeWarnings', []);
      item.set('data.disableReasons.corruptInstall', false);
      item.set('data.disableReasons.suspiciousInstall', false);
      item.set('data.disableReasons.updateRequired', false);
      Polymer.dom.flush();
      testWarningVisible('#runtime-warnings', false);
      testWarningVisible('#corrupted-warning', false);
      testWarningVisible('#suspicious-warning', false);
      testWarningVisible('#blacklisted-warning', false);
      testWarningVisible('#update-required-warning', false);
    });
  });

  return {
    suiteName: suiteName,
    TestNames: TestNames,
  };
});
