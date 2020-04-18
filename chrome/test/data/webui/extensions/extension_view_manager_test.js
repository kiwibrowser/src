// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for extensions-detail-view. */
cr.define('extension_view_manager_tests', function() {
  /** @enum {string} */
  var TestNames = {
    Visibility: 'visibility',
    EventFiring: 'event firing',
  };

  var viewManager;
  var views;
  var suiteName = 'ExtensionViewManagerTest';

  suite(suiteName, function() {
    // Initialize an extension item before each test.
    setup(function() {
      PolymerTest.clearBody();

      document.body.innerHTML = `
        <extensions-view-manager id="viewManager">
          <div slot="view" id="viewOne">view 1</div>
          <div slot="view" id="viewTwo">view 2</div>
          <div slot="view" id="viewThree">view 3</div>
        </extensions-view-manager>`;

      viewManager = document.body.querySelector('#viewManager');
    });

    test(assert(TestNames.Visibility), function() {
      function assertViewVisible(id, isVisible) {
        const expectFunc = isVisible ? expectTrue : expectFalse;
        expectFunc(extension_test_util.isVisible(viewManager, '#' + id, true));
      }

      assertViewVisible('viewOne', false);
      assertViewVisible('viewTwo', false);
      assertViewVisible('viewThree', false);

      return viewManager.switchView('viewOne')
          .then(() => {
            assertViewVisible('viewOne', true);
            assertViewVisible('viewTwo', false);
            assertViewVisible('viewThree', false);

            return viewManager.switchView('viewThree');
          })
          .then(() => {
            assertViewVisible('viewOne', false);
            assertViewVisible('viewTwo', false);
            assertViewVisible('viewThree', true);
          });
    });

    test(assert(TestNames.EventFiring), function() {
      var viewOne = viewManager.querySelector('#viewOne');
      var enterStart = false;
      var enterFinish = false;
      var exitStart = false;
      var exitFinish = false;

      var fired = {};

      ['view-enter-start', 'view-enter-finish', 'view-exit-start',
       'view-exit-finish',
      ].forEach(type => {
        viewOne.addEventListener(type, () => {
          fired[type] = true;
        });
      });

      // Setup the switch promise first.
      var enterPromise = viewManager.switchView('viewOne');
      // view-enter-start should fire synchronously.
      expectTrue(!!fired['view-enter-start']);
      // view-enter-finish should not fire yet.
      expectFalse(!!fired['view-enter-finish']);

      return enterPromise
          .then(() => {
            // view-enter-finish should fire after animation.
            expectTrue(!!fired['view-enter-finish']);

            enterPromise = viewManager.switchView('viewTwo');
            // view-exit-start should fire synchronously.
            expectTrue(!!fired['view-exit-start']);
            // view-exit-finish should not fire yet.
            expectFalse(!!fired['view-exit-finish']);

            return enterPromise;
          })
          .then(() => {
            // view-exit-finish should fire after animation.
            expectTrue(!!fired['view-exit-finish']);
          });
    });
  });

  return {
    suiteName: suiteName,
    TestNames: TestNames,
  };
});
