// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for extension-item. */
cr.define('extension_item_list_tests', function() {
  /** @enum {string} */
  var TestNames = {
    Filtering: 'item list filtering',
    NoItemsMsg: 'empty item list',
    NoSearchResultsMsg: 'empty item list filtering results',
  };

  var suiteName = 'ExtensionItemListTest';

  suite(suiteName, function() {
    /** @type {extensions.ItemList} */
    var itemList;
    var testVisible;

    // Initialize an extension item before each test.
    setup(function() {
      PolymerTest.clearBody();
      itemList = new extensions.ItemList();
      testVisible = extension_test_util.testVisible.bind(null, itemList);

      var createExt = extension_test_util.createExtensionInfo;
      var extensionItems = [
        createExt({name: 'Alpha', id: 'a'.repeat(32)}),
        createExt({name: 'Bravo', id: 'b'.repeat(32)}),
        createExt({name: 'Charlie', id: 'c'.repeat(32)})
      ];
      var appItems = [
        createExt({name: 'QQ', id: 'q'.repeat(32)}),
      ];
      itemList.extensions = extensionItems;
      itemList.apps = appItems;
      itemList.filter = '';
      document.body.appendChild(itemList);
    });

    test(assert(TestNames.Filtering), function() {
      function itemLengthEquals(num) {
        Polymer.dom.flush();
        expectEquals(
            itemList.shadowRoot.querySelectorAll('extensions-item').length,
            num);
      }

      // We should initially show all the items.
      itemLengthEquals(4);

      // All extension items have an 'a'.
      itemList.filter = 'a';
      itemLengthEquals(3);
      // Filtering is case-insensitive, so all extension items should be shown.
      itemList.filter = 'A';
      itemLengthEquals(3);
      // Only 'Bravo' has a 'b'.
      itemList.filter = 'b';
      itemLengthEquals(1);
      expectEquals('Bravo', itemList.$$('extensions-item').data.name);
      // Test inner substring (rather than prefix).
      itemList.filter = 'lph';
      itemLengthEquals(1);
      expectEquals('Alpha', itemList.$$('extensions-item').data.name);
      // Test trailing/leading spaces.
      itemList.filter = '   Alpha  ';
      itemLengthEquals(1);
      expectEquals('Alpha', itemList.$$('extensions-item').data.name);
      // Test string with no matching items.
      itemList.filter = 'z';
      itemLengthEquals(0);
      // A filter of '' should reset to show all items.
      itemList.filter = '';
      itemLengthEquals(4);
      // A filter of 'q' should should show just the apps item.
      itemList.filter = 'q';
      itemLengthEquals(1);
    });

    test(assert(TestNames.NoItemsMsg), function() {
      Polymer.dom.flush();
      testVisible('#no-items', false);
      testVisible('#no-search-results', false);

      itemList.extensions = [];
      itemList.apps = [];
      Polymer.dom.flush();
      testVisible('#no-items', true);
      testVisible('#no-search-results', false);
    });

    test(assert(TestNames.NoSearchResultsMsg), function() {
      Polymer.dom.flush();
      testVisible('#no-items', false);
      testVisible('#no-search-results', false);

      itemList.filter = 'non-existent name';
      Polymer.dom.flush();
      testVisible('#no-items', false);
      testVisible('#no-search-results', true);
    });
  });

  return {
    suiteName: suiteName,
    TestNames: TestNames,
  };
});
