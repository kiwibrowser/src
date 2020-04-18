// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for extension-sidebar. */
cr.define('extension_sidebar_tests', function() {
  /** @enum {string} */
  var TestNames = {
    LayoutAndClickHandlers: 'layout and click handlers',
    SetSelected: 'set selected',
  };

  var suiteName = 'ExtensionSidebarTest';

  suite(suiteName, function() {
    /** @type {extensions.Sidebar} */
    var sidebar;

    setup(function() {
      PolymerTest.clearBody();
      sidebar = new extensions.Sidebar();
      document.body.appendChild(sidebar);
    });

    test(assert(TestNames.SetSelected), function() {
      const selector = '.section-item.iron-selected';
      expectFalse(!!sidebar.$$(selector));

      window.history.replaceState(undefined, '', '/shortcuts');
      PolymerTest.clearBody();
      sidebar = new extensions.Sidebar();
      document.body.appendChild(sidebar);
      Polymer.dom.flush();
      expectEquals(sidebar.$$(selector).id, 'sections-shortcuts');

      window.history.replaceState(undefined, '', '/');
      PolymerTest.clearBody();
      sidebar = new extensions.Sidebar();
      document.body.appendChild(sidebar);
      Polymer.dom.flush();
      expectEquals(sidebar.$$(selector).id, 'sections-extensions');
    });

    test(assert(TestNames.LayoutAndClickHandlers), function(done) {
      extension_test_util.testIcons(sidebar);

      var testVisible = extension_test_util.testVisible.bind(null, sidebar);
      testVisible('#sections-extensions', true);
      testVisible('#sections-shortcuts', true);
      testVisible('#more-extensions', true);

      sidebar.isSupervised = true;
      Polymer.dom.flush();
      testVisible('#more-extensions', false);

      var currentPage;
      extensions.navigation.addListener(newPage => {
        currentPage = newPage;
      });

      MockInteractions.tap(sidebar.$$('#sections-shortcuts'));
      expectDeepEquals(currentPage, {page: Page.SHORTCUTS});

      MockInteractions.tap(sidebar.$$('#sections-extensions'));
      expectDeepEquals(currentPage, {page: Page.LIST});

      // Clicking on the link for the current page should close the dialog.
      sidebar.addEventListener('close-drawer', () => done());
      MockInteractions.tap(sidebar.$$('#sections-extensions'));
    });
  });

  return {
    suiteName: suiteName,
    TestNames: TestNames,
  };
});
