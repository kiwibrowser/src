// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for media-router-container that focus on the
 * first run flow.
 */
cr.define('media_router_container_first_run_flow', function() {
  function registerTests() {
    suite('MediaRouterContainerFirstRunFlow', function() {
      /**
       * Checks whether the elements specified in |elementIdList| are visible.
       * Checks whether all other elements are not visible. Throws an assertion
       * error if this is not true.
       *
       * @param {!Array<!string>} elementIdList List of id's of elements that
       *     should be visible.
       */
      var checkElementsVisibleWithId;

      /**
       * Checks the visibility of an element with |elementId| in |container|.
       * An element is considered visible if it exists and its |hidden| property
       * is |false|.
       *
       * @param {boolean} visible Whether the element should be visible.
       * @param {!string} elementId The id of the element to test.
       */
      var checkElementVisibleWithId;

      /**
       * Media Router Container created before each test.
       * @type {?MediaRouterContainer}
       */
      var container;

      /**
       * The list of CastModes to show.
       * @type {!Array<!media_router.CastMode>}
       */
      var fakeCastModeList = [];

      /**
       * The list of CastModes to show with non-PRESENTATION modes only.
       * @type {!Array<!media_router.CastMode>}
       */
      var fakeCastModeListWithNonPresentationModesOnly = [];

      // Import media_router_container.html before running suite.
      suiteSetup(function() {
        return PolymerTest.importHtml(
            'chrome://media-router/elements/media_router_container/' +
            'media_router_container.html');
      });

      setup(function(done) {
        PolymerTest.clearBody();
        // Initialize a media-router-container before each test.
        container = document.createElement('media-router-container');
        document.body.appendChild(container);

        // Get common functions and variables.
        var test_base = media_router_container_test_base.init(container);

        checkElementsVisibleWithId = test_base.checkElementsVisibleWithId;
        checkElementVisibleWithId = test_base.checkElementVisibleWithId;
        fakeCastModeList = test_base.fakeCastModeList;
        fakeCastModeListWithNonPresentationModesOnly =
            test_base.fakeCastModeListWithNonPresentationModesOnly;

        container.castModeList = test_base.fakeCastModeList;

        // Allow for the media router container to be created, attached, and
        // listeners registered in an afterNextRender() call.
        Polymer.RenderStatus.afterNextRender(this, done);
      });

      // Tests for 'acknowledge-first-run-flow' event firing when the
      // 'first-run-button' button is clicked and the cloud preference checkbox
      // is not shown.
      test('first run button click', function(done) {
        container.showFirstRunFlow = true;

        setTimeout(function() {
          container.addEventListener(
              'acknowledge-first-run-flow', function(data) {
                assertEquals(undefined, data.detail.optedIntoCloudServices);
                done();
              });
          MockInteractions.tap(
              container.shadowRoot.getElementById('first-run-button'));
        });
      });

      // Tests for 'acknowledge-first-run-flow' event firing when the
      // 'first-run-button' button is clicked and the cloud preference checkbox
      // is also shown.
      test('first run button with cloud pref click', function(done) {
        container.showFirstRunFlow = true;
        container.showFirstRunFlowCloudPref = true;

        setTimeout(function() {
          container.addEventListener(
              'acknowledge-first-run-flow', function(data) {
                assertTrue(data.detail.optedIntoCloudServices);
                done();
              });
          MockInteractions.tap(
              container.shadowRoot.getElementById('first-run-button'));
        });
      });

      // Tests for 'acknowledge-first-run-flow' event firing when the
      // 'first-run-button' button is clicked after the cloud preference
      // checkbox is deselected.
      test('first run button with cloud pref deselected click', function(done) {
        container.showFirstRunFlow = true;
        container.showFirstRunFlowCloudPref = true;

        setTimeout(function() {
          container.addEventListener(
              'acknowledge-first-run-flow', function(data) {
                assertFalse(data.detail.optedIntoCloudServices);
                done();
              });
          MockInteractions.tap(
              container.shadowRoot.getElementById('first-run-cloud-checkbox'));
          MockInteractions.tap(
              container.shadowRoot.getElementById('first-run-button'));
        });
      });

      // Tests for the expected visible UI when interacting with the first run
      // flow.
      test('first run button visibility', function(done) {
        container.showFirstRunFlow = true;

        setTimeout(function() {
          checkElementVisibleWithId(true, 'first-run-flow');
          MockInteractions.tap(
              container.shadowRoot.getElementById('first-run-button'));

          setTimeout(function() {
            checkElementVisibleWithId(false, 'first-run-flow');
            done();
          });
        });
      });

      // Tests for the expected visible UI when interacting with the first run
      // flow with cloud services preference.
      test('first run button visibility', function(done) {
        container.showFirstRunFlow = true;
        container.showFirstRunFlowCloudPref = true;

        setTimeout(function() {
          checkElementsVisibleWithId([
            'container-header', 'device-missing', 'first-run-flow',
            'first-run-flow-cloud-pref', 'sink-list-view'
          ]);
          MockInteractions.tap(
              container.shadowRoot.getElementById('first-run-button'));

          setTimeout(function() {
            checkElementsVisibleWithId(
                ['container-header', 'device-missing', 'sink-list-view']);
            done();
          });
        });
      });
    });
  }

  return {
    registerTests: registerTests,
  };
});
