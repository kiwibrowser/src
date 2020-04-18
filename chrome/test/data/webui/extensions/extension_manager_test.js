// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for extension-sidebar. */
cr.define('extension_manager_tests', function() {
  /** @enum {string} */
  var TestNames = {
    ChangePages: 'change pages',
    ItemListVisibility: 'item list visibility',
    SplitItems: 'split items',
    UrlNavigationToDetails: 'url navigation to details',
  };

  function getDataByName(list, name) {
    return assert(list.find(function(el) {
      return el.name == name;
    }));
  }

  var suiteName = 'ExtensionManagerTest';

  suite(suiteName, function() {
    /** @type {extensions.Manager} */
    var manager;

    /** @param {string} viewElement */
    function assertViewActive(tagName) {
      expectTrue(!!manager.$.viewManager.querySelector(`${tagName}.active`));
    }

    setup(function() {
      manager = document.querySelector('extensions-manager');
    });

    test(assert(TestNames.ItemListVisibility), function() {
      var extension = getDataByName(manager.extensions_, 'My extension 1');

      var list = manager.$['items-list'];
      var listHasItemWithName = (name) =>
          !!list.extensions.find(el => el.name == name);

      expectEquals(manager.extensions_, manager.$['items-list'].extensions);
      expectTrue(listHasItemWithName('My extension 1'));

      manager.removeItem_(extension.id);
      Polymer.dom.flush();
      expectFalse(listHasItemWithName('My extension 1'));

      manager.addItem_('extensions_', extension);
      Polymer.dom.flush();
      expectTrue(listHasItemWithName('My extension 1'));
    });

    test(assert(TestNames.SplitItems), function() {
      var sectionHasItemWithName = function(section, name) {
        return !!manager[section].find(function(el) {
          return el.name == name;
        });
      };

      // Test that we properly split up the items into two sections.
      expectTrue(sectionHasItemWithName('extensions_', 'My extension 1'));
      expectTrue(sectionHasItemWithName(
          'apps_', 'Platform App Test: minimal platform app'));
      expectTrue(sectionHasItemWithName('apps_', 'hosted_app'));
      expectTrue(sectionHasItemWithName('apps_', 'Packaged App Test'));
    });

    test(assert(TestNames.ChangePages), function() {
      MockInteractions.tap(
          manager.$$('extensions-toolbar').$$('cr-toolbar').$$('#menuButton'));
      Polymer.dom.flush();

      // We start on the item list.
      MockInteractions.tap(manager.$$('#sidebar').$['sections-extensions']);
      Polymer.dom.flush();
      assertViewActive('extensions-item-list');

      // Switch: item list -> keyboard shortcuts.
      MockInteractions.tap(manager.$$('#sidebar').$['sections-shortcuts']);
      Polymer.dom.flush();
      assertViewActive('extensions-keyboard-shortcuts');

      // Switch: item list -> detail view.
      var item = manager.$['items-list'].$$('extensions-item');
      assert(item);
      item.onDetailsTap_();
      Polymer.dom.flush();
      assertViewActive('extensions-detail-view');

      // Switch: detail view -> keyboard shortcuts.
      MockInteractions.tap(manager.$$('#sidebar').$['sections-shortcuts']);
      Polymer.dom.flush();
      assertViewActive('extensions-keyboard-shortcuts');

      // We get back on the item list.
      MockInteractions.tap(manager.$$('#sidebar').$['sections-extensions']);
      Polymer.dom.flush();
      assertViewActive('extensions-item-list');
    });

    test(assert(TestNames.UrlNavigationToDetails), function() {
      assertViewActive('extensions-detail-view');
      var detailsView = manager.$$('extensions-detail-view');
      expectEquals('ldnnhddmnhbkjipkidpdiheffobcpfmf', detailsView.data.id);

      // Try to open detail view for invalid ID.
      extensions.navigation.navigateTo(
          {page: Page.DETAILS, extensionId: 'z'.repeat(32)});
      Polymer.dom.flush();
      // Should be re-routed to the main page.
      assertViewActive('extensions-item-list');

      // Try to open detail view with a valid ID.
      extensions.navigation.navigateTo({
        page: Page.DETAILS,
        extensionId: 'ldnnhddmnhbkjipkidpdiheffobcpfmf'
      });
      Polymer.dom.flush();
      assertViewActive('extensions-detail-view');
    });
  });

  return {
    suiteName: suiteName,
    TestNames: TestNames,
  };
});
